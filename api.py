#!/bin/env python3

import subprocess
import threading
import queue
import json
import sys
import urllib.parse
import io
import os
import time
import uuid
import base64
from flask import Flask, request, Response, stream_with_context, jsonify
from flask_cors import CORS

app = Flask(__name__)
CORS(app)

# Global variables
engine_state = "INIT"
engine_process = None
active_model_path = None
input_queue = queue.Queue()
output_queue = queue.Queue()

def run_engine(binary, model_path, np, ctx):
    global engine_state, engine_process, active_model_path
    
    cmd = f"{binary} -m {model_path} -ngl 99 -sm row -fa -np {np} -c {ctx}"
    print('[ENGINE] Starting process:', cmd)
    engine_process = subprocess.Popen(cmd, shell=True, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, bufsize=1, errors='ignore')
    active_model_path = model_path

    def handle_input():
        while True:
            prompt = input_queue.get()
            print('got prompt:', prompt)
            if prompt is None:  # Signal to stop the thread
                engine_process.stdin.write(f"\n")
                engine_process.stdin.flush()
                break
            else:
                engine_process.stdin.write(f"{prompt}\n")
                engine_process.stdin.flush()

    input_thread = threading.Thread(target=handle_input)
    input_thread.start()

    for line in engine_process.stdout:
        # print('DEBUG:', line.strip())
        if 'LOADING' in line and engine_state != 'LOADING':
            engine_state = "LOADING"
            print('engine_state =', engine_state)
        elif line.startswith("INPUT:"):
            engine_state = "READY"
            print('engine_state =', engine_state)
        else:
            output_queue.put(line)
            
    print('[ENGINE] Process exited.')

    input_queue.put(None)  # Signal input thread to stop
    input_thread.join()

def get_model_name(model_path):
    return os.path.basename(model_path)

def encode_prompt_params(prompt, n, max_tokens):
    """Encode prompt parameters with || delimiter"""
    params = f"{prompt}||{n}||{max_tokens}"
    return urllib.parse.quote(params)

def process_request_streaming(prompt, n, max_tokens):
    global engine_state
    
    completion_id = f"cmpl-{str(uuid.uuid4())}"
    encoded_prompt = encode_prompt_params(prompt, n, max_tokens)
    input_queue.put(encoded_prompt)
    
    def emit_event(item):
        if item['event'] == 'done':
            return 'data: [DONE]\n\n'

        choice = {'index': item['index'], 'text': item['text'], 'finish_reason': None}
        
        if item['event'] == 'stop':
            if item['hit_stop'] == 1:
                choice['finish_reason'] = 'stop'
                choice['stop_reason'] = choice.pop('text')
                choice['text'] = None
            else:
                choice['finish_reason'] = 'length'

        response = {
            "id": completion_id,
            "object": "text_completion", 
            "created": int(time.time()),
            "model": get_model_name(active_model_path),
            "choices": [choice]            
        }
        
        print('emit_event', item, response)

        return 'data: ' + json.dumps(response) + "\n\n"
    
    while True:
        line = output_queue.get()
        if line.startswith("START:"):
            engine_state = "RUNNING"
            print('engine_state =', engine_state)
            # yield emit_event({"event": "start", "count": int(line.split(":")[1])})
        elif line.startswith("STREAM"):
            parts = line.strip().split(":")
            index = int(parts[0][-1])
            text = parts[1]
            decoded_text = urllib.parse.unquote(text)
            yield emit_event({"event": "stream", "index": int(index), "text": decoded_text})
        elif line.startswith("STOP"):
            parts = line.split(":")
            index = int(parts[0][-1])
            text = parts[1]
            length = int(parts[2])
            hit_stop = int(parts[3])
            decoded_text = urllib.parse.unquote(text)
            yield emit_event({"event": "stop", "index": index, "length": length, "text": decoded_text, "hit_stop": hit_stop})
        elif line.startswith("DONE:"):
            yield emit_event({"event": "done", "count": int(line.split(":")[1])})
            break

@app.route('/health', methods=['GET'])
def health():
    if engine_state != "READY":
        return jsonify({"error": "Engine not ready"}), 500
    
    return jsonify({"error": "Engine ready."}), 200
    
@app.route('/v1/models', methods=['GET'])
def models():
    global active_model_path, engine_state
    
    if engine_state != "READY":
        return jsonify({"error": "Engine not ready"}), 500
            
    return jsonify({'data': [{ 'id': active_model_path, 'engine_state': engine_state }]})
    
@app.route('/v1/completions', methods=['POST'])
def completions():
    global engine_state
    
    if engine_state != "READY":
        return {"error": "Engine not ready"}, 500
    
    data = request.json
    prompt = data.get('prompt', '')
    stream = data.get('stream', False)
    n = data.get('n', 1)  # Number of completions
    max_tokens = data.get('max_tokens', 64)  # Max tokens to generate
    
    if stream:
        return Response(stream_with_context(process_request_streaming(prompt, n, max_tokens)), content_type='text/event-stream; charset=utf-8')
    else:
        return process_request_non_streaming(prompt, n, max_tokens)

def process_request_non_streaming(prompt, n, max_tokens):
    global engine_state
    
    completion_id = f"cmpl-{str(uuid.uuid4())}"
    encoded_prompt = encode_prompt_params(prompt, n, max_tokens)
    input_queue.put(encoded_prompt)
    
    sequences = []
    while True:
        line = output_queue.get()
        if line.startswith("SEQUENCE"):
            parts = line.strip().split(":")
            index = int(parts[0][-1])
            encoded_text = parts[1].strip()
            decoded_text = urllib.parse.unquote(encoded_text)
            sequences.append({"index": index, "text": decoded_text})
        elif line.startswith("DONE:"):
            break
    
    choices = []
    for seq in sequences:
        choices.append({
            "text": seq["text"],
            "index": seq["index"],
            "finish_reason": "stop",
            "logprobs": None
        })
    
    response = {
        "id": completion_id,
        "object": "text_completion",
        "created": int(time.time()),
        "model": get_model_name(active_model_path),
        "choices": choices
    }
    
    return jsonify(response)

def startup(model:str, engine:str="llama.cpp/build/bin/llama-batched", n:int=8, ctx:int=8192, port:int=9090):
    # Start the engine in a separate thread
    engine_thread = threading.Thread(target=run_engine, args=(engine, model, n, ctx))
    engine_thread.start()
    
    # Run the Flask app
    app.run(host='0.0.0.0', port=port)
        
if __name__ == '__main__':
    from fire import Fire
    Fire(startup)

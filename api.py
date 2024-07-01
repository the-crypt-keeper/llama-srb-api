import subprocess
import threading
import queue
import json
import sys
import urllib.parse
from flask import Flask, request, Response, stream_with_context, jsonify

app = Flask(__name__)

# Global variables
engine_state = "INIT"
engine_process = None
active_model_path = None
input_queue = queue.Queue()
output_queue = queue.Queue()

def run_engine(model_path, gpus, np):
    global engine_state, engine_process, active_model_path
    
    cmd = f"CUDA_VISIBLE_DEVICES={gpus} ./llama-batched -m {model_path} -ngl 99 -fa -np {np} 2>&1"
    engine_process = subprocess.Popen(cmd, shell=True, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, bufsize=1)
    active_model_path = model_path
           
    def handle_input():
        while True:
            prompt = input_queue.get()
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
        if 'llm_load_tensors' in line and engine_state != 'LOADING':
            engine_state = "LOADING"
            print('engine_state =', engine_state)
        elif line.startswith("INPUT:"):
            engine_state = "READY"
            print('engine_state =', engine_state)
        output_queue.put(line)
        if 't/s' in line: print(line)

    input_queue.put(None)  # Signal input thread to stop
    input_thread.join()

def process_request(prompt):
    global engine_state
    
    encoded_prompt = urllib.parse.quote(prompt)
    input_queue.put(encoded_prompt)
    
    def emit_event(item):
        if item['event'] in ['start','done']: print(item)
        return 'data: ' + json.dumps(item) + "\n"
    
    while True:
        line = output_queue.get()
        # print(line)
        if line.startswith("START:"):
            engine_state = "RUNNING"
            print('engine_state =', engine_state)
            yield emit_event({"event": "start", "count": int(line.split(":")[1])})
        elif line.startswith("STREAM"):
            parts = line.strip().split(":")
            index = int(parts[0][-1])
            text = parts[1]
            decoded_text = urllib.parse.unquote(text)
            yield emit_event({"event": "stream", "index": int(index), "text": decoded_text})
        elif line.startswith("STOP"):
            parts = line.split(":")
            index = int(parts[0][-1])
            length = int(parts[1])
            stop = int(parts[2])            
            yield emit_event({"event": "stop", "index": index, "length": length, "stop": stop})
        elif line.startswith("DONE:"):
            yield emit_event({"event": "done", "count": int(line.split(":")[1])})
            break

@app.route('/v1/models', methods=['GET'])
def models():
    global active_model_path, engine_state
    return jsonify({'data': [{ 'id': active_model_path, 'engine_state': engine_state }]})
    
@app.route('/v1/completions', methods=['POST'])
def completions():
    global engine_state
    
    if engine_state != "READY":
        return {"error": "Engine not ready"}, 500
    
    data = request.json
    prompt = data.get('prompt', '')
    
    return Response(stream_with_context(process_request(prompt)))

if __name__ == '__main__':
    # Start the engine in a separate thread
    engine_thread = threading.Thread(target=run_engine, args=("/home/mike/models/L3-Aethora-15B-V2.Q6_K.gguf", "0", 8))
    engine_thread.start()
    
    # Run the Flask app
    app.run(host='0.0.0.0')
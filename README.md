# llama-srb-api

The `llama-server` supports batching but 1) only across requests and 2) without sharing KV cache or prompt processing.  The `n` parameter to the completions API is ignored.

This repository contains a pile of hacks to turn `llama-batched` into the back-end of an API server that can do Single Request Batching: for one prompt, return multiple completions.

This is useful in Creative Writing applications.

# News

*1/11*
- Update to c05e8c9934f94fde49bc1bc9dc51eed282605150 of upstream llama.cpp
- Improve OpenAI compatibility in the streaming responces, the stop token is no longer echoed as `text` but instead as `stop_reason`
- Fix last token being cut off when finish_reason was length

*1/4*
- Update to b56f079e28fda692f11a8b59200ceb815b05d419 of upstream llama.cpp
- Add /health endpoint (will 500 until model has loaded)
- Support request stream parameter (frue/false)
- Support request n parameter (can be up to the -np server launched with)
- Support request max_tokens parameter

The `stop` sequence is still hard-coded which makes this backend difficult to use in the general case but that's coming next.

*11/11* 

- Update to 9f409893519b4a6def46ef80cd6f5d05ac0fb157 of upstream llama.cpp
   * *-sm row memory usage improvements* KV caches are now split up so higher context can be realized.
  * *XTC/DRY sampling* Use command line to set-up the sampler of your choice.

# TODO

- Add `/v1/chat/completions` endpoint support
- Allow `temperature`, `top_k` and other sampler controls to be set from the request layer (instead of server launch time)
- Allow `stop` sequences and `min_tokens` to be set at the request layer (instead of hard-coding in the cpp)

# Starting

1. If you need support for `CUDA_ARCHITECTURES` other then `61`, edit `build.sh` and modify appropriately.

2. Run `build.sh` to clone llama.cpp, apply the patch and build the binary. 

3. Use `api.py` to launch server: `python3 ./api.py --model ~/models/openhermes-2.5-mistral-7b.Q5_0.gguf`

By default port will be `9090` you can use --port parameter to override.

By default the maximum number of completions will be `8`, you can use the --n parameter to override.

# Using

## Non-streaming Mode

```
$ curl -X POST -H 'Content-type: application/json' http://localhost:9090/v1/completions -d '{ "prompt": "Once upon a time,", "n": 4, "max_tokens": 128, "stream": false }'
{
    "choices": [
        {
            "finish_reason": "stop",
            "index": 0,
            "logprobs": null,
            "text": " I was the biggest, most annoying fan in the world."
        },
        {
            "finish_reason": "stop",
            "index": 1,
            "logprobs": null,
            "text": " there lived a beautiful girl named Rani."
        },
        {
            "finish_reason": "stop",
            "index": 2,
            "logprobs": null,
            "text": " there was a small boy named Jack."
        },
        {
            "finish_reason": "stop",
            "index": 3,
            "logprobs": null,
            "text": " a boy named Alex lived in a small town in Montana."
        }
    ],
    "created": 1736032145,
    "id": "cmpl-af64842f-4349-4db1-812a-7c47c253c546",
    "model": "Celeste-12B-V1.6.Q6_K.gguf",
    "object": "text_completion"
}
```

## Streaming mode

```
$ curl -X POST -H 'Content-type: application/json' http://localhost:9090/v1/completions -d '{ "prompt": "Once upon a time,", "n": 4, "max_tokens": 128, "stream": true }'
{"id": "cmpl-971c9be8-1465-4a6d-a5c1-3289ee9be759", "object": "text_completion", "created": 1736032194, "model": "Celeste-12B-V1.6.Q6_K.gguf", "choices": [{"event": "stream", "index": 0, "text": " there", "finish_reason": null, "stop_reason": null}]}
{"id": "cmpl-971c9be8-1465-4a6d-a5c1-3289ee9be759", "object": "text_completion", "created": 1736032194, "model": "Celeste-12B-V1.6.Q6_K.gguf", "choices": [{"event": "stream", "index": 1, "text": " it", "finish_reason": null, "stop_reason": null}]}
{"id": "cmpl-971c9be8-1465-4a6d-a5c1-3289ee9be759", "object": "text_completion", "created": 1736032194, "model": "Celeste-12B-V1.6.Q6_K.gguf", "choices": [{"event": "stream", "index": 2, "text": " there", "finish_reason": null, "stop_reason": null}]}
{"id": "cmpl-971c9be8-1465-4a6d-a5c1-3289ee9be759", "object": "text_completion", "created": 1736032194, "model": "Celeste-12B-V1.6.Q6_K.gguf", "choices": [{"event": "stream", "index": 3, "text": " there", "finish_reason": null, "stop_reason": null}]}
{"id": "cmpl-971c9be8-1465-4a6d-a5c1-3289ee9be759", "object": "text_completion", "created": 1736032194, "model": "Celeste-12B-V1.6.Q6_K.gguf", "choices": [{"event": "stream", "index": 0, "text": " was", "finish_reason": null, "stop_reason": null}]}
{"id": "cmpl-971c9be8-1465-4a6d-a5c1-3289ee9be759", "object": "text_completion", "created": 1736032194, "model": "Celeste-12B-V1.6.Q6_K.gguf", "choices": [{"event": "stream", "index": 1, "text": " was", "finish_reason": null, "stop_reason": null}]}
{"id": "cmpl-971c9be8-1465-4a6d-a5c1-3289ee9be759", "object": "text_completion", "created": 1736032194, "model": "Celeste-12B-V1.6.Q6_K.gguf", "choices": [{"event": "stream", "index": 2, "text": " was", "finish_reason": null, "stop_reason": null}]}
{"id": "cmpl-971c9be8-1465-4a6d-a5c1-3289ee9be759", "object": "text_completion", "created": 1736032194, "model": "Celeste-12B-V1.6.Q6_K.gguf", "choices": [{"event": "stream", "index": 3, "text": " was", "finish_reason": null, "stop_reason": null}]}
{"id": "cmpl-971c9be8-1465-4a6d-a5c1-3289ee9be759", "object": "text_completion", "created": 1736032194, "model": "Celeste-12B-V1.6.Q6_K.gguf", "choices": [{"event": "stream", "index": 0, "text": " a", "finish_reason": null, "stop_reason": null}]}
{"id": "cmpl-971c9be8-1465-4a6d-a5c1-3289ee9be759", "object": "text_completion", "created": 1736032194, "model": "Celeste-12B-V1.6.Q6_K.gguf", "choices": [{"event": "stream", "index": 1, "text": " a", "finish_reason": null, "stop_reason": null}]}
{"id": "cmpl-971c9be8-1465-4a6d-a5c1-3289ee9be759", "object": "text_completion", "created": 1736032194, "model": "Celeste-12B-V1.6.Q6_K.gguf", "choices": [{"event": "stream", "index": 2, "text": " a", "finish_reason": null, "stop_reason": null}]}
{"id": "cmpl-971c9be8-1465-4a6d-a5c1-3289ee9be759", "object": "text_completion", "created": 1736032194, "model": "Celeste-12B-V1.6.Q6_K.gguf", "choices": [{"event": "stream", "index": 3, "text": " a", "finish_reason": null, "stop_reason": null}]}
{"id": "cmpl-971c9be8-1465-4a6d-a5c1-3289ee9be759", "object": "text_completion", "created": 1736032194, "model": "Celeste-12B-V1.6.Q6_K.gguf", "choices": [{"event": "stream", "index": 0, "text": " little", "finish_reason": null, "stop_reason": null}]}
{"id": "cmpl-971c9be8-1465-4a6d-a5c1-3289ee9be759", "object": "text_completion", "created": 1736032194, "model": "Celeste-12B-V1.6.Q6_K.gguf", "choices": [{"event": "stream", "index": 1, "text": " time", "finish_reason": null, "stop_reason": null}]}
{"id": "cmpl-971c9be8-1465-4a6d-a5c1-3289ee9be759", "object": "text_completion", "created": 1736032194, "model": "Celeste-12B-V1.6.Q6_K.gguf", "choices": [{"event": "stream", "index": 2, "text": " boy", "finish_reason": null, "stop_reason": null}]}
{"id": "cmpl-971c9be8-1465-4a6d-a5c1-3289ee9be759", "object": "text_completion", "created": 1736032194, "model": "Celeste-12B-V1.6.Q6_K.gguf", "choices": [{"event": "stream", "index": 3, "text": " great", "finish_reason": null, "stop_reason": null}]}
...
{"id": "cmpl-971c9be8-1465-4a6d-a5c1-3289ee9be759", "object": "text_completion", "created": 1736032195, "model": "Celeste-12B-V1.6.Q6_K.gguf", "choices": [{"event": "stop", "index": 0, "length": 14, "stop": 1, "finish_reason": null, "stop_reason": null}]}
{"id": "cmpl-971c9be8-1465-4a6d-a5c1-3289ee9be759", "object": "text_completion", "created": 1736032195, "model": "Celeste-12B-V1.6.Q6_K.gguf", "choices": [{"event": "stop", "index": 1, "length": 17, "stop": 1, "finish_reason": null, stop_reason": null}]}
{"id": "cmpl-971c9be8-1465-4a6d-a5c1-3289ee9be759", "object": "text_completion", "created": 1736032195, "model": "Celeste-12B-V1.6.Q6_K.gguf", "choices": [{"event": "stop", "index": 2, "length": 19, "stop": 1, "finish_reason": null, "stop_reason": null}]}
{"id": "cmpl-971c9be8-1465-4a6d-a5c1-3289ee9be759", "object": "text_completion", "created": 1736032195, "model": "Celeste-12B-V1.6.Q6_K.gguf", "choices": [{"event": "stop", "index": 3, "length": 22, "stop": 1, "finish_reason": null, "stop_reason": null}]}
[DONE]

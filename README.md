# llama-srb-api

The `llama-server` supports batching but 1) only across requests and 2) without sharing KV cache or prompt processing.  The `n` parameter to the completions API is ignored.

This repository contains a pile of hacks to turn `llama-batched` into the back-end of an API server that can do Single Request Batching: for one prompt, return multiple completions.

This is useful in Creative Writing applications.

# Starting

Run `build.sh` to clone llama.cpp. It can be any version, but currently aimed at [PR #8215](https://github.com/ggerganov/llama.cpp/pull/8215).

Then use `api.py` to launch server:

python3 ./api.py --model ~/models/openhermes-2.5-mistral-7b.Q5_0.gguf

By default port will be `9090` you can use --port parameter to override.

By default the number of completions will be `8`, you can use the --n parameter to override.

# Using

```
$ curl -X POST -H 'Content-type: application/json' http://localhost:9090/v1/completions -d '{ "prompt": "Once upon a time," }'
data: {"event": "start", "count": 8}
data: {"event": "stream", "index": 0, "text": " there"}
data: {"event": "stream", "index": 1, "text": " I"}
data: {"event": "stream", "index": 2, "text": " there"}
data: {"event": "stream", "index": 3, "text": " in"}
data: {"event": "stream", "index": 4, "text": " back"}
data: {"event": "stream", "index": 5, "text": " I"}
data: {"event": "stream", "index": 6, "text": " on"}
data: {"event": "stream", "index": 7, "text": " in"}
data: {"event": "stream", "index": 0, "text": " was"}
data: {"event": "stream", "index": 1, "text": " wrote"}
data: {"event": "stream", "index": 2, "text": " was"}
data: {"event": "stream", "index": 3, "text": " the"}
data: {"event": "stream", "index": 4, "text": " when"}
data: {"event": "stream", "index": 5, "text": " was"}
data: {"event": "stream", "index": 6, "text": " the"}
data: {"event": "stream", "index": 7, "text": " a"}
data: {"event": "stream", "index": 0, "text": " a"}
data: {"event": "stream", "index": 1, "text": " about"}
data: {"event": "stream", "index": 2, "text": " a"}
data: {"event": "stream", "index": 3, "text": " early"}
data: {"event": "stream", "index": 4, "text": " I"}
data: {"event": "stream", "index": 5, "text": " an"}
data: {"event": "stream", "index": 6, "text": " sh"}
data: {"event": "stream", "index": 7, "text": " country"}
data: {"event": "stream", "index": 0, "text": " small"}
data: {"event": "stream", "index": 1, "text": " what"}
data: {"event": "stream", "index": 2, "text": " beautiful"}
data: {"event": "stream", "index": 3, "text": " days"}
data: {"event": "stream", "index": 4, "text": " was"}
data: {"event": "stream", "index": 5, "text": " av"}
data: {"event": "stream", "index": 6, "text": "ores"}
data: {"event": "stream", "index": 7, "text": " far"}
data: {"event": "stream", "index": 0, "text": ","}
data: {"event": "stream", "index": 1, "text": " a"}
data: {"event": "stream", "index": 2, "text": " lady"}
data: {"event": "stream", "index": 3, "text": " of"}
data: {"event": "stream", "index": 4, "text": " a"}
data: {"event": "stream", "index": 5, "text": "id"}
data: {"event": "stream", "index": 6, "text": " of"}
data: {"event": "stream", "index": 7, "text": ","}
data: {"event": "stream", "index": 0, "text": " dust"}
data: {"event": "stream", "index": 1, "text": " difference"}
data: {"event": "stream", "index": 2, "text": " named"}
data: {"event": "stream", "index": 3, "text": " digital"}
data: {"event": "stream", "index": 4, "text": " young"}
data: {"event": "stream", "index": 5, "text": " cycl"}
data: {"event": "stream", "index": 6, "text": " the"}
data: {"event": "stream", "index": 7, "text": " far"}
data: {"event": "stream", "index": 0, "text": "y"}
data: {"event": "stream", "index": 1, "text": " just"}
data: {"event": "stream", "index": 2, "text": " Ch"}
data: {"event": "stream", "index": 3, "text": " music"}
data: {"event": "stream", "index": 4, "text": " lad"}
data: {"event": "stream", "index": 5, "text": "ist"}
data: {"event": "stream", "index": 6, "text": " Mediterranean"}
data: {"event": "stream", "index": 7, "text": " away"}
data: {"event": "stream", "index": 0, "text": " corner"}
data: {"event": "stream", "index": 1, "text": " an"}
data: {"event": "stream", "index": 2, "text": "ant"}
data: {"event": "stream", "index": 3, "text": ","}
data: {"event": "stream", "index": 4, "text": ","}
data: {"event": "stream", "index": 5, "text": "."}
data: {"event": "stop", "index": 5, "length": 7, "stop": 1}
data: {"event": "stream", "index": 6, "text": ","}
data: {"event": "stream", "index": 7, "text": ","}
data: {"event": "stream", "index": 0, "text": " in"}
data: {"event": "stream", "index": 1, "text": " hour"}
data: {"event": "stream", "index": 2, "text": "elle"}
data: {"event": "stream", "index": 3, "text": " a"}
data: {"event": "stream", "index": 4, "text": " we"}
data: {"event": "stream", "index": 6, "text": " there"}
data: {"event": "stream", "index": 7, "text": " there"}
data: {"event": "stream", "index": 0, "text": " the"}
data: {"event": "stream", "index": 1, "text": " of"}
data: {"event": "stream", "index": 2, "text": "."}
data: {"event": "stop", "index": 2, "length": 9, "stop": 1}
data: {"event": "stream", "index": 3, "text": " company"}
data: {"event": "stream", "index": 4, "text": " would"}
data: {"event": "stream", "index": 6, "text": " was"}
data: {"event": "stream", "index": 7, "text": " lived"}
data: {"event": "stream", "index": 0, "text": " back"}
data: {"event": "stream", "index": 1, "text": " writing"}
data: {"event": "stream", "index": 3, "text": " named"}
data: {"event": "stream", "index": 4, "text": " have"}
data: {"event": "stream", "index": 6, "text": " a"}
data: {"event": "stream", "index": 7, "text": " a"}
data: {"event": "stream", "index": 0, "text": " of"}
data: {"event": "stream", "index": 1, "text": " a"}
data: {"event": "stream", "index": 3, "text": " i"}
data: {"event": "stream", "index": 4, "text": " to"}
data: {"event": "stream", "index": 6, "text": " land"}
data: {"event": "stream", "index": 7, "text": " princess"}
data: {"event": "stream", "index": 0, "text": " our"}
data: {"event": "stream", "index": 1, "text": " day"}
data: {"event": "stream", "index": 3, "text": "Mesh"}
data: {"event": "stream", "index": 4, "text": " go"}
data: {"event": "stream", "index": 6, "text": " where"}
data: {"event": "stream", "index": 7, "text": "."}
data: {"event": "stop", "index": 7, "length": 12, "stop": 1}
data: {"event": "stream", "index": 0, "text": " living"}
data: {"event": "stream", "index": 1, "text": " could"}
data: {"event": "stream", "index": 3, "text": " offered"}
data: {"event": "stream", "index": 4, "text": " down"}
data: {"event": "stream", "index": 6, "text": " the"}
data: {"event": "stream", "index": 0, "text": " room"}
data: {"event": "stream", "index": 1, "text": " make"}
data: {"event": "stream", "index": 3, "text": " a"}
data: {"event": "stream", "index": 4, "text": " to"}
data: {"event": "stream", "index": 6, "text": " sun"}
data: {"event": "stream", "index": 0, "text": " where"}
data: {"event": "stream", "index": 1, "text": "."}
data: {"event": "stop", "index": 1, "length": 15, "stop": 1}
data: {"event": "stream", "index": 3, "text": " way"}
data: {"event": "stream", "index": 4, "text": " the"}
data: {"event": "stream", "index": 6, "text": " always"}
data: {"event": "stream", "index": 0, "text": " I"}
data: {"event": "stream", "index": 3, "text": " to"}
data: {"event": "stream", "index": 4, "text": " video"}
data: {"event": "stream", "index": 6, "text": " sh"}
data: {"event": "stream", "index": 0, "text": " would"}
data: {"event": "stream", "index": 3, "text": " share"}
data: {"event": "stream", "index": 4, "text": " store"}
data: {"event": "stream", "index": 6, "text": "one"}
data: {"event": "stream", "index": 0, "text": " go"}
data: {"event": "stream", "index": 3, "text": " and"}
data: {"event": "stream", "index": 4, "text": " and"}
data: {"event": "stream", "index": 6, "text": "."}
data: {"event": "stop", "index": 6, "length": 18, "stop": 1}
data: {"event": "stream", "index": 0, "text": " to"}
data: {"event": "stream", "index": 3, "text": " download"}
data: {"event": "stream", "index": 4, "text": " search"}
data: {"event": "stream", "index": 0, "text": " think"}
data: {"event": "stream", "index": 3, "text": " music"}
data: {"event": "stream", "index": 4, "text": " through"}
data: {"event": "stream", "index": 0, "text": ","}
data: {"event": "stream", "index": 3, "text": " for"}
data: {"event": "stream", "index": 4, "text": " their"}
data: {"event": "stream", "index": 0, "text": " pray"}
data: {"event": "stream", "index": 3, "text": " free"}
data: {"event": "stream", "index": 4, "text": " shelves"}
data: {"event": "stream", "index": 0, "text": ","}
data: {"event": "stream", "index": 3, "text": "."}
data: {"event": "stop", "index": 3, "length": 23, "stop": 1}
data: {"event": "stream", "index": 4, "text": " to"}
data: {"event": "stream", "index": 0, "text": " and"}
data: {"event": "stream", "index": 4, "text": " find"}
data: {"event": "stream", "index": 0, "text": " write"}
data: {"event": "stream", "index": 4, "text": " a"}
data: {"event": "stream", "index": 0, "text": " in"}
data: {"event": "stream", "index": 4, "text": " movie"}
data: {"event": "stream", "index": 0, "text": " my"}
data: {"event": "stream", "index": 4, "text": " to"}
data: {"event": "stream", "index": 0, "text": " journal"}
data: {"event": "stream", "index": 4, "text": " rent"}
data: {"event": "stream", "index": 0, "text": "."}
data: {"event": "stop", "index": 0, "length": 29, "stop": 1}
data: {"event": "stream", "index": 4, "text": "."}
data: {"event": "stop", "index": 4, "length": 29, "stop": 1}
data: {"event": "done", "count": 8}
```

## Events

### start/done

Emitted at the very start and very end of inference

### stream

Emitted for each token in each stream

### stop

Emitted when a stream is terminated, `stop` indicates an early termination due to stop tokens.

# Enhancements

- Move temperature, top_k and other sampler controls up the API layer instead of hard-coding them
- Accept n from API server instead of hard-coding (this might be tough given the internal architecture - maybe need a max_n instead)
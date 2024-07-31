#!/bin/bash

git clone https://github.com/ggerganov/llama.cpp.git build
cd build
git checkout 268c5660062270a2c19a36fc655168aa287aaec2
git apply ../llama-batch-api.patch
make LLAMA_CUDA=1 llama-batched -j
cd -

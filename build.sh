#!/bin/bash

git clone https://github.com/ggerganov/llama.cpp.git build
cd build
git checkout 9f409893519b4a6def46ef80cd6f5d05ac0fb157
git apply ../llama-batch-api.patch
make LLAMA_CUDA=1 llama-batched -j
cd -

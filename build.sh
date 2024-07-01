#!/bin/bash

git clone https://github.com/JohannesGaessler/llama.cpp.git build
cd build
git checkout cuda-iq-opt-3
git apply ../llama-batch-api.patch
make LLAMA_CUDA=1 llama-batched -j
cd -

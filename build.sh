#!/bin/bash

git clone https://github.com/ggerganov/llama.cpp.git
cd llama.cpp
git checkout b56f079e28fda692f11a8b59200ceb815b05d419
git apply ../llama-batch-api.patch
rm -rf build
cmake -B build -DGGML_CUDA=1 -DCMAKE_CUDA_ARCHITECTURES=61
cmake --build build --config Release --target llama-batched -j
cd -

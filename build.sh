#!/bin/bash

git clone https://github.com/ggerganov/llama.cpp.git
cd llama.cpp
git checkout c05e8c9934f94fde49bc1bc9dc51eed282605150
git apply ../llama-batch-api.patch
rm -rf build
cmake -B build -DGGML_CUDA=1 -DCMAKE_CUDA_ARCHITECTURES=61
cmake --build build --config Release --target llama-batched -j
cd -

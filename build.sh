#!/bin/bash
set -xe

# 1. Compile the Compute Shader to SPIR-V
echo "Compiling shader..."
glslangValidator -V lat_comp.comp -o lat_comp.comp.spv

# 2. Compile and Link the C++ Modular Project
echo "Compiling M4 Max Profiler..."
clang++ -std=c++17 \
    main.cc \
    gpu_system.cc \
    memory_block.cc \
    shader_pipeline.cc \
    timer.cc \
    utils.cc \
    -o m4_profiler \
    -I/opt/homebrew/include \
    -L/opt/homebrew/lib \
    -lvulkan

echo "Build Complete. Run with: ./m4_profiler"

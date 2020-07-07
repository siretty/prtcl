#!/bin/bash

sudo pacman -S git lua clang cmake boost openmp eigen catch2 gtest lua-lpeg

git clone https://github.com/siretty/prtcl prtcl

cd prtcl
git submodule update --init --recursive

mkdir build

CC=clang CXX=clang++ cmake -S. -Bbuild -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS_RELEASE=-march=native


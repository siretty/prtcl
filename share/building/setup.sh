#!/bin/bash

sudo pacman --needed -S git lua clang cmake boost openmp eigen catch2 gtest lua-lpeg ninja htop

git clone https://github.com/siretty/prtcl prtcl

cd prtcl
git submodule update --init --recursive

mkdir build

CC=clang CXX=clang++ cmake -GNinja -S. -Bbuild -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS_RELEASE=-march=native

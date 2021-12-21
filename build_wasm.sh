#!/bin/bash

set -e
set -o pipefail

echo building wasm files...
cd ./wasm
# 1. turn source file into llvm IR bitcode
clang -cc1 -Ofast -emit-llvm-bc -triple=wasm32-unknown-unknown-wasm -std=c11 -fvisibility hidden gunfight.c

# NOTE(fede): if we have serveral files we neeed to link them all here and optimize this

# 2. compile the llvm IR into an object file
llc -O3 -filetype=obj gunfight.bc -o gunfight.o

# 3. link with llc
wasm-ld --no-entry gunfight.o -o ../build/gunfight.wasm --strip-all -allow-undefined-file js_imports.txt --import-memory --export-dynamic

# TODO(fede): check wasm-opt
cd ../
echo OK

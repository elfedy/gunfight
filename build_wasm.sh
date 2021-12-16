#!/bin/bash

set -e
set -o pipefail

echo building wasm files...
cd ./wasm
clang --target=wasm32 -nostdlib -Wl,--no-entry -Wl,--export-all -Wl,--import-memory -o ../build/gunfight.wasm gunfight.cpp
cd ../
echo OK

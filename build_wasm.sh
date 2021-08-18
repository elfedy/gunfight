#!/bin/bash

set -e
set -o pipefail

echo building wasm files...
cd ./wasm
cargo build --target wasm32-unknown-unknown --release
cd ../
cp ./wasm/target/wasm32-unknown-unknown/release/gunf.wasm ./build/gunfight.wasm

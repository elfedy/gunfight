#!/bin/bash

set -e
set -o pipefail

echo Starting build...


# Reset the build directory
rm -rf build
mkdir -p build

./build_assets.sh
./build_wasm.sh

#!/bin/bash

set -e
set -o pipefail

echo Starting build...


# Reset the build directory
rm -rf build
mkdir -p build

./build_web.sh
./build_wasm.sh

# Copy assets
cp assets/*png build/

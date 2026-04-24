#!/bin/bash

set -e
set -o pipefail

echo Starting build...

node ./dev/build_sprite_atlas.js
node ./dev/build_charset.js

# Reset the build directory
rm -rf build
mkdir -p build

./build_web.sh
./build_wasm.sh

# Copy assets
cp assets/* build/

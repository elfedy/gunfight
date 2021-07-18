#!/bin/bash

set -e
set -o pipefail

echo Starting build...

echo Checking build dependencies...
# TODO(Fede): check for typescript version and give an error
# if it is incorrect.
if command -v tsc >/dev/null 2>&1; then
  echo Typescript is already installed.
else
  echo Did not find Typescript
  echo Installing...
  npm install -g typescript
fi

# Reset the build directory
rm -rf build
mkdir -p build

echo Compiling .ts files...
tsFiles='
  web/shaders.ts
  web/shaders_color.ts
  web/index.ts
  '
tsc $tsFiles --outFile build/index.js --lib dom,es2015
echo .ts Files compiled successfully

echo building wasm files...
cd ./wasm
cargo build --target wasm32-unknown-unknown --release
cd ../

echo copying source files...
cp ./wasm/target/wasm32-unknown-unknown/release/gunf.wasm ./build/gunfight.wasm
cp ./web/index.html ./build/index.html

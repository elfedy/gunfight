#!/bin/bash

set -e
set -o pipefail

# TODO(Fede): check for typescript version and give an error
# if it is incorrect.
if command -v tsc >/dev/null 2>&1; then
  echo Typescript is already installed.
else
  echo Did not find Typescript
  echo Installing...
  npm install -g typescript
fi

tsFiles='
  web/math.ts
  web/shaders.ts
  web/shaders_color.ts
  web/index.ts
  '
tsc $tsFiles --outFile build/index.js --lib dom,es2015

cp ./web/index.html ./build/index.html

#!/bin/bash

set -e
set -o pipefail

echo building web...

tsFiles='
  web/math.ts
  web/shaders.ts
  web/shaders_color.ts
  web/shaders_texture.ts
  web/index.ts
  '
tsc $tsFiles --outFile build/index.js --lib dom,es2015

cp ./web/index.html ./build/index.html

echo OK

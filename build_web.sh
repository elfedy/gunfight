#!/bin/bash

set -e
set -o pipefail

echo building web...

cp ./web/index.html ./build/index.html

# Save current directory
CUR_DIR=$(pwd)

# Target repo path
OTHER_REPO="web"

{
    cd "$OTHER_REPO"
    npm run build
} || true

cd "$CUR_DIR"

echo OK

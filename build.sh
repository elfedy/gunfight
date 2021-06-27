cargo build --target wasm32-unknown-unknown --release
cp target/wasm32-unknown-unknown/release/gunf.wasm ./build/gunfight.wasm
cp index.html ./build/index.html
cp index.js ./build/index.js

WebAssembly.instantiateStreaming(fetch('gunfight.wasm'), {})
.then(obj => {
  console.log(obj);
});

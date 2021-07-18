// TODO: The compiled wasm asks for initial 17 so it cannot be smaller than that. But where does the 17 come from?
let wasmMemory = new WebAssembly.Memory({initial: 17, maximum: 100});
let wasmMemoryBuffer = new Uint8Array(wasmMemory.buffer);

function consoleLogUtf8FromMemory(start, offset) {
  let arr = wasmMemoryBuffer.subarray( start, start + offset);
  let decoder = new TextDecoder("utf-8");
  console.log(decoder.decode(arr));
}


WebAssembly.instantiateStreaming(
  fetch('gunfight.wasm'),
  {
    env: {
      memory: wasmMemory,

      console_log_utf8_from_memory: (start, offset) => consoleLogUtf8FromMemory(start, offset),
    }
  }
).then(wasm => {
  // TODO(Draw a triangle using the vertices sent from wasm web gl);
  run(wasm);
});

function run(wasm) {
  console.log(wasm);
  wasm.instance.exports.say_hello();

  // Initialize web gl
  let canvas = <HTMLCanvasElement> document.getElementById('canvas');
  let gl = canvas.getContext('webgl');
  if(gl === null) {
    alert("Unable to initialize WebGL. Your browser or machine may not support it.");
    return;
  }
  
  // Setup Shaders
  let colorShaderInfo = colorShaderSetup(gl);
  console.log(colorShaderInfo);
}

let wasmMemory = new WebAssembly.Memory({initial: 17, maximum: 17});
let wasmMemoryBuffer = new Uint8Array(wasmMemory.buffer);

WebAssembly.instantiateStreaming(
  fetch('gunfight.wasm'),
  {
    env: {
      memory: wasmMemory,

      console_log_bytes: consoleLogBytes,
      console_log_utf8: consoleLogUtf8,
    }
  }
).then(wasm => {
  // TODO(Draw a triangle using the vertices sent from wasm web gl);
  run(wasm);
});

function consoleLogBytes(start, offset) {
  console.log(start);
  let arr = wasmMemoryBuffer.subarray( start, start + offset);
  console.log(arr);
}

function consoleLogUtf8(start, offset) {
  let arr = wasmMemoryBuffer.subarray( start, start + offset);
  let decoder = new TextDecoder("utf-8");
  console.log(decoder.decode(arr));
}

function run(wasm) {
  wasm.instance.exports.write_vertices();
  let pointer = wasm.instance.exports.buffer_pointer();
  let arr = wasmMemoryBuffer.subarray(pointer, pointer + 6 * 4);
  let buffer_slice = wasmMemory.buffer.slice(pointer, pointer + 6 * 4);
  let float32_buffer = new Float32Array(buffer_slice);

  //console.log(wasmMemoryBuffer[pointer]);

  // Initialize web gl
  let canvas = <HTMLCanvasElement> document.getElementById('canvas');
  let gl = canvas.getContext('webgl');
  if(gl === null) {
    alert("Unable to initialize WebGL. Your browser or machine may not support it.");
    return;
  }
  
  // Setup Shaders
  let colorShaderInfo = colorShaderSetup(gl);
  gl.clearColor(0.9, 0.9, 0.9, 1);
  gl.clear(gl.COLOR_BUFFER_BIT);

  // Tell WebGL how to convert from clip space to pixels
  gl.viewport(0, 0, gl.canvas.width, gl.canvas.height);

  gl.useProgram(colorShaderInfo.program);
  gl.bindBuffer(gl.ARRAY_BUFFER, colorShaderInfo.buffers.aPosition);

  /*
  gl.vertexAttribPointer(
    colorShaderInfo.locations.aPosition,
    2,  // size: components per iteration
    gl.FLOAT,  // data type
    false, // normalize
    0, // stride: bytes between beggining of consecutive vetex attributes in buffer
    0 // offset: where to start reading data from the buffer
  );
  */
  gl.vertexAttribPointer(
    colorShaderInfo.locations.aPosition,
    2,  // size: components per iteration
    gl.FLOAT,  // data type
    false, // normalize
    0, // stride: bytes between beggining of consecutive vetex attributes in buffer
    0 // offset: where to start reading data from the buffer
  );

  // Enable vertex attribute
  gl.enableVertexAttribArray(colorShaderInfo.locations.aPosition);

  let matrixProjection = Mat3.projection(gl.canvas.width, gl.canvas.height);
  let matrix = Mat3.identity();
  matrix = Mat3.multiply(matrix, matrixProjection);
  gl.uniformMatrix3fv(colorShaderInfo.locations.uMatrix, false, matrix);


  let color = {r: 1, g: 0, b: 0, alpha: 1.0};
  let colorArray = [color.r, color.g, color.b, color.alpha]
  gl.uniform4fv(colorShaderInfo.locations.uColor, colorArray);

  // Add vertices to array buffer
  // TODO: this should work in theory
  console.log(arr);
  gl.bufferData(gl.ARRAY_BUFFER, float32_buffer, gl.STATIC_DRAW);

  let offset = 0;
  let count = 3;
  gl.drawArrays(gl.TRIANGLES, offset, count);

}

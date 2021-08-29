let wasmMemory = new WebAssembly.Memory({initial: 17, maximum: 17});
let wasmMemoryBuffer = new Uint8Array(wasmMemory.buffer);

WebAssembly.instantiateStreaming(
  fetch('gunfight.wasm'),
  {
    env: {
      memory: wasmMemory,

      console_log_bytes: consoleLogBytes,
      console_log_utf8: consoleLogUtf8,
      get_canvas_width: getCanvasWidth,
      get_canvas_height: getCanvasHeight,
    }
  }
).then(wasm => {
  // TODO(Draw a triangle using the vertices sent from wasm web gl);
  run(wasm);
});

// WASM IMPORTS
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

function getCanvasWidth() {
  let canvas = <HTMLCanvasElement> document.getElementById('canvas');
  return canvas.width;
}

function getCanvasHeight() {
  let canvas = <HTMLCanvasElement> document.getElementById('canvas');
  return canvas.height;
}

// RUN
function run(wasm) {
  let canvas = <HTMLCanvasElement> document.getElementById('canvas');

  let dims = new Uint32Array([canvas.width, canvas.height]);
  wasm.instance.exports.game_update_and_render();
  let colorShaderEntitiesCount = wasm.instance.exports.color_shader_entities_count();

  let verticesPointer = wasm.instance.exports.color_shader_vertices_pointer();
  // NOTE: Asumes squared entites which involves 6 pairs of vertices of 4 bytes each (float32)
  // hello
  let verticesSlice = 
    wasmMemory.buffer.slice(
      verticesPointer,
      verticesPointer + colorShaderEntitiesCount * 6 * 2 * 4
    );
  let verticesBuffer = new Float32Array(verticesSlice);

  let colorsPointer = wasm.instance.exports.color_shader_colors_pointer();
  let colorsSlice = 
    wasmMemory.buffer.slice(
      colorsPointer,
      colorsPointer + colorShaderEntitiesCount * 4 * 4 // 4 float32s per color
    );
  let colorsBuffer = new Float32Array(colorsSlice);

  // Initialize web gl
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


  // Add vertices to array buffer
  gl.bufferData(gl.ARRAY_BUFFER, verticesBuffer, gl.STATIC_DRAW);
  console.log(verticesBuffer);

  console.log(colorsBuffer);
  // TODO: multiple rectangles not working
  for(let i = 0; i < colorShaderEntitiesCount; i++) {
    /*
    let colorArray = [
      colorsBuffer[4 * i],
      colorsBuffer[4 * i + 1],
      colorsBuffer[4 * i + 2],
      colorsBuffer[4 * i + 3],
    ]
    */
    let colorArray = colorsBuffer.slice(4 * i, 4 * i + 4);
    console.log(colorArray);
    gl.uniform4fv(colorShaderInfo.locations.uColor, colorArray);


    let offset = 6 * i;
    let count = 6;
    gl.drawArrays(gl.TRIANGLES, offset, count);
  }

}

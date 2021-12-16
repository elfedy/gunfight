// NOTE(fede): 1 page = 64 KB
// Rust asks for 16 pages (1 MB) of memory for the stack by default
let wasmMemory = new WebAssembly.Memory({initial: 160, maximum: 160});
let wasmMemoryBuffer = new Uint8Array(wasmMemory.buffer);

WebAssembly.instantiateStreaming(
  fetch('gunfight.wasm'),
  {
    env: {
      memory: wasmMemory,

      console_log_bytes: consoleLogBytes,
      console_log_utf8: consoleLogUtf8,
      console_log_pointer: (ptr) => console.log(ptr),
      console_log_usize: (val) => console.log(val),
      get_canvas_width: getCanvasWidth,
      get_canvas_height: getCanvasHeight,
    }
  }
).then(wasm => {
  main(wasm);
});

function main(wasm) {
  let heapBase = wasm.instance.exports.getHeapBase();
  //window.requestAnimationFrame(run(wasm));
  let timestamp = 0;
  console.log(heapBase);
  // TODO: draw the triangle
  let canvas = <HTMLCanvasElement> document.getElementById('canvas');


  /*
  let colorsPointer = wasm.instance.exports.color_shader_colors_pointer();
  let colorsSlice = 
    wasmMemory.buffer.slice(
      colorsPointer,
      colorsPointer + colorShaderEntitiesCount * 4 * 4 // 4 float32s per color
    );
  let colorsBuffer = new Float32Array(colorsSlice);
  */

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

  // UPDATE AND RENDER
  wasm.instance.exports.updateAndRender(timestamp); 
    
  // Tell WebGL how to convert from clip space to pixels
  gl.viewport(0, 0, gl.canvas.width, gl.canvas.height);

  gl.useProgram(colorShaderInfo.program);

  // Enable vertex attribute
  gl.enableVertexAttribArray(colorShaderInfo.locations.aPosition);

  gl.bindBuffer(gl.ARRAY_BUFFER, colorShaderInfo.buffers.aPosition);

  // Specify how to pull the data from the buffer
  gl.vertexAttribPointer(
    colorShaderInfo.locations.aPosition,
    2,  // size: components per iteration
    gl.FLOAT,  // data type
    false, // normalize
    0, // stride: bytes between beggining of consecutive vetex attributes in buffer
    0 // offset: where to start reading data from the buffer
  );

  let matrixProjection = Mat3.projection(gl.canvas.width, gl.canvas.height);
  gl.uniformMatrix3fv(colorShaderInfo.locations.uMatrix, false, matrixProjection);

  // Add vertices to array buffer
  let verticesSlice = 
    wasmMemory.buffer.slice(
      heapBase,
      heapBase + 3 * 2 * 4 // three vertices, two f32 coordinates
    );
  let verticesBuffer = new Float32Array(verticesSlice);
  gl.bufferData(gl.ARRAY_BUFFER, verticesBuffer, gl.STATIC_DRAW);

  let colorArray = new Float32Array([1.0, 1.0, 0, 1.0]);
  gl.uniform4fv(colorShaderInfo.locations.uColor, colorArray);

  let offset = 0;
  let count = 3;
  gl.drawArrays(gl.TRIANGLES, offset, count);
}

function run(wasm) {
  return (timestamp) => {
    window.requestAnimationFrame(run(wasm));
    wasm.instance.exports.updateAndRender(timestamp); 
    let triangleBase = wasm.instance.exports.getHeapBase();
  }
}

// WASM IMPORTS
function consoleLogBytes(start, offset) {
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
/*
function gameLoopFn(wasm) {
  // TODO: Game hangs here for some reason when looping with requestAnimationFrame
  let loop = function(timestamp) {
    let canvas = <HTMLCanvasElement> document.getElementById('canvas');

    wasm.instance.exports.game_update_and_render(timestamp);
    let colorShaderEntitiesCount = wasm.instance.exports.color_shader_entities_count();

    let verticesPointer = wasm.instance.exports.color_shader_vertices_pointer();
    // NOTE: Asumes squared entites which involves 6 pairs of vertices of 4 bytes each (float32)
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

    for(let i = 0; i < colorShaderEntitiesCount; i++) {
      let colorArray = colorsBuffer.slice(4 * i, 4 * i + 4);
      gl.uniform4fv(colorShaderInfo.locations.uColor, colorArray);


      let offset = 6 * i;
      let count = 6;
      gl.drawArrays(gl.TRIANGLES, offset, count);
      
      //requestAnimationFrame(loop);
    }
  }

  return loop;
}

function main(wasm) {
  let gameLoop = gameLoopFn(wasm);
  requestAnimationFrame(gameLoop);
}
*/

// Global Config
const GlobalConfig = {
  debug: false,
};

// NOTE(fede): 1 page = 64 KB
let wasmMemory = new WebAssembly.Memory({initial: 160, maximum: 160});
let wasmMemoryBuffer = new Uint8Array(wasmMemory.buffer);

WebAssembly.instantiateStreaming(
  fetch('gunfight.wasm'),
  {
    env: {
      memory: wasmMemory,

      logBytes: consoleLogBytes,
      logUtf8: consoleLogUtf8,
      logFloat32: (f32) => console.log(f32),
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
  // TODO: draw the triangle
  let canvas = <HTMLCanvasElement> document.getElementById('canvas');


  // Initialize web gl
  let gl = canvas.getContext('webgl');
  if(gl === null) {
    alert("Unable to initialize WebGL. Your browser or machine may not support it.");
    return;
  }

  // Setup Shaders
  let colorShaderInfo = colorShaderSetup(gl);


  // Setup event listeners
	const processKeyChange = (keyCode, isDown) => {
		let keyIndex = getKeyIndex(keyCode);
		
		if (keyIndex !== null) {
			wasm.instance.exports.processControllerInput(keyIndex, isDown);
		}
	}
  window.addEventListener('keydown', (e) => processKeyChange(e.code, 1));
  window.addEventListener('keyup', (e) => processKeyChange(e.code, 0));
 
  // UPDATE AND RENDER
  window.requestAnimationFrame(run(wasm, gl, colorShaderInfo));
}


function getKeyIndex(keyCode) {
    switch(keyCode) {
      case 'ArrowUp': {
				return 0;
      } break;
      case 'ArrowDown': {
				return 1;
      } break;
      case 'ArrowRight': {
				return 2;
      } break;
      case 'ArrowLeft': {
				return 3;
      } break;
      case 'Space': {
				return 4;
      } break;
      case 'KeyP': {
				return 5;
      } break;
			default: {
				return null;
			}	
    }
}

function run(wasm, gl, colorShaderInfo) {
  return (frameTimestamp) => {
    let DEBUGTimestamp = frameTimestamp;
    wasm.instance.exports.updateAndRender(frameTimestamp); 
    DEBUGTimestamp = DEBUGTime("Update", DEBUGTimestamp);

    gl.clearColor(0.9, 0.9, 0.9, 1);
    gl.clear(gl.COLOR_BUFFER_BIT);
      
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

    let numberOfTriangles = wasm.instance.exports.getTrianglesCount();
    let numberOfVertices = numberOfTriangles * 3;
    let pointsPerVertex = 2;
    let bytesPerFloat32 = 4;

    // Add vertices to array buffer
    let vertexBufferBase = wasm.instance.exports.getVertexBufferBase();
    let vertexSlice = 
      wasmMemory.buffer.slice(
        vertexBufferBase,
        vertexBufferBase + numberOfVertices * pointsPerVertex * bytesPerFloat32
      );
    let vertexBuffer = new Float32Array(vertexSlice);
    gl.bufferData(gl.ARRAY_BUFFER, vertexBuffer, gl.STATIC_DRAW);

    let colorBufferUnitSize = 4 * bytesPerFloat32; // 4 f32s pre color
    let colorBufferBase = wasm.instance.exports.getColorBufferBase();
    let colorSlice = 
      wasmMemory.buffer.slice(
        colorBufferBase,
        colorBufferBase + numberOfTriangles * colorBufferUnitSize // 4 float32s per color
      );
    let colorArray = new Float32Array(colorSlice);

    DEBUGTimestamp = DEBUGTime("Shader Setup", DEBUGTimestamp);

    // Draw Triangles one by one so that we can set the color
    for(let triangleIndex = 0; triangleIndex < numberOfTriangles; triangleIndex++) {

      let colorSliceOffset = triangleIndex * 4;
      let color = colorArray.slice(colorSliceOffset, colorSliceOffset + 4);

      gl.uniform4fv(colorShaderInfo.locations.uColor, color);

      let verticesToDraw = 3;
      let vertexBufferOffset = triangleIndex * 3;
      gl.drawArrays(gl.TRIANGLES, vertexBufferOffset, verticesToDraw);

    }
    DEBUGTimestamp = DEBUGTime("Draw Triangles", DEBUGTimestamp);

    DEBUGTime("Frame Total", frameTimestamp);
    window.requestAnimationFrame(run(wasm, gl, colorShaderInfo));
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

function DEBUG(log: string) {
  if (GlobalConfig.debug) {
    console.log(log);
  }
}

function DEBUGTime(log: string, start: number): number {
  let end = performance.now();
  if (GlobalConfig.debug) {
    let duration = end - start;
    console.log(`${log}: ${duration}ms`)
  }

  return end;
}

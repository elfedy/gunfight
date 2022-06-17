// Global Config
const GlobalConfig = {
  debug: false,
};

// NOTE(fede): 1 page = 64 KB
let wasmMemory = new WebAssembly.Memory({initial: 160, maximum: 160});
let wasmMemoryBuffer = new Uint8Array(wasmMemory.buffer);

let spriteAtlas = new Image();
let context = {
  wasm: null,
  images: [],
  loadedImages: 0,
}

spriteAtlas.onload = () =>  {
  context.images.push(spriteAtlas);
  context.loadedImages++;
};
spriteAtlas.src = 'sprite_atlas.png';


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
  context.wasm = wasm;
  waitForInitialization();
});

function waitForInitialization() {
  if(context.wasm !== null && context.loadedImages === 1) {
    initializeGameLoop(context);
  } else {
    setTimeout(waitForInitialization, 100);
  }
}

function initializeGameLoop(context) {
  let wasm = context.wasm;
  let canvas = <HTMLCanvasElement> document.getElementById('canvas');


  // Initialize web gl
  let gl = canvas.getContext('webgl');
  if(gl === null) {
    alert("Unable to initialize WebGL. Your browser or machine may not support it.");
    return;
  }
  gl.pixelStorei(gl.UNPACK_FLIP_Y_WEBGL, true);

  // Setup Shaders
  let colorShaderInfo = colorShaderSetup(gl);
  let textureShaderInfo = textureShaderSetup(gl);
  textureShaderSetTexture(gl, 'TEXTURE0', textureShaderInfo, context.images[0]);

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
  window.requestAnimationFrame(run(wasm, gl, colorShaderInfo, textureShaderInfo));
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

function run(wasm, gl, colorShaderInfo, textureShaderInfo) {
  return (frameTimestamp) => {
    window.requestAnimationFrame(run(wasm, gl, colorShaderInfo, textureShaderInfo));

    let DEBUGTimestamp = frameTimestamp;
    wasm.instance.exports.updateAndRender(frameTimestamp); 
    DEBUGTimestamp = DEBUGTime("Update", DEBUGTimestamp);

    gl.clearColor(0.9, 0.9, 0.9, 1);
    gl.clear(gl.COLOR_BUFFER_BIT);
      
    // Tell WebGL how to convert from clip space to pixels
    gl.viewport(0, 0, gl.canvas.width, gl.canvas.height);

    gl.useProgram(colorShaderInfo.program);

    // COLOR SHADER

    // Bind aPosition to the array buffer target
    gl.bindBuffer(gl.ARRAY_BUFFER, colorShaderInfo.buffers.aPosition);

    // Specify how to pull the data from the buffer currently set to the ARRAY BUFFER target
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

    let numberOfTriangles = wasm.instance.exports.colorShaderGetTrianglesCount();
    let numberOfVertices = numberOfTriangles * 3;
    let pointsPerVertex = 2;
    let bytesPerFloat32 = 4;

    // Add vertices to array buffer
    let vertexBufferBase = wasm.instance.exports.getBufferBase(0);
    let vertexSlice = 
      wasmMemory.buffer.slice(
        vertexBufferBase,
        vertexBufferBase + numberOfVertices * pointsPerVertex * bytesPerFloat32
      );
    let vertexBuffer = new Float32Array(vertexSlice);
    // Populate buffer bound to array buffer with the vetices needed to be drawn
    gl.bufferData(gl.ARRAY_BUFFER, vertexBuffer, gl.STATIC_DRAW);

    let colorBufferUnitSize = 4 * bytesPerFloat32; // 4 f32s per color
    let colorBufferBase = wasm.instance.exports.getBufferBase(1);
    let colorSlice = 
      wasmMemory.buffer.slice(
        colorBufferBase,
        colorBufferBase + numberOfTriangles * colorBufferUnitSize
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

    // TEXTURE SHADER
    gl.useProgram(textureShaderInfo.program);

    let textureShaderNumberOfTriangles = wasm.instance.exports.textureShaderGetTrianglesCount();
    let textureShaderNumberOfVertices = textureShaderNumberOfTriangles * 3;

    // Provide position coordinates
    gl.bindBuffer(gl.ARRAY_BUFFER, textureShaderInfo.buffers.aPosition);
    gl.vertexAttribPointer(
      textureShaderInfo.locations.aPosition,
      2,  // size: components per iteration
      gl.FLOAT,  // data type
      false, // normalize
      0, // stride: bytes between beggining of consecutive vetex attributes in buffer
      0 // offset: where to start reading data from the buffer
    );
    // Add vertices to array buffer
    let textureShaderAPositionBufferBase = wasm.instance.exports.getBufferBase(2);
    let textureShaderAPositionBufferEnd = textureShaderAPositionBufferBase + textureShaderNumberOfVertices * pointsPerVertex * bytesPerFloat32;
    let textureShaderAPositionSlice = 
      wasmMemory.buffer.slice(
        textureShaderAPositionBufferBase,
        textureShaderAPositionBufferEnd
      );
    let aPositionValues = new Float32Array(textureShaderAPositionSlice);
    // Populate buffer bound to array buffer with the vetices needed to be drawn
    gl.bufferData(gl.ARRAY_BUFFER, aPositionValues, gl.STATIC_DRAW);

    // Provide texture coordinates
    gl.bindBuffer(gl.ARRAY_BUFFER, textureShaderInfo.buffers.aTexCoord);
    gl.vertexAttribPointer(
      textureShaderInfo.locations.aTexCoord,
      2,  // size: components per iteration
      gl.FLOAT,  // data type
      false, // normalize
      0, // stride: bytes between beggining of consecutive vetex attributes in buffer
      0 // offset: where to start reading data from the buffer
    );

    // Add vertices to array buffer
    let textureShaderATexCoordBufferBase = wasm.instance.exports.getBufferBase(3);
    let textureShaderATexCoordBufferEnd = textureShaderATexCoordBufferBase + textureShaderNumberOfVertices * pointsPerVertex * bytesPerFloat32;
    let textureShaderATexCoordSlice = 
      wasmMemory.buffer.slice(
        textureShaderATexCoordBufferBase,
        textureShaderATexCoordBufferEnd
      );
    let aTexCoordValues = new Float32Array(textureShaderATexCoordSlice);
    gl.bufferData(gl.ARRAY_BUFFER, aTexCoordValues, gl.STATIC_DRAW);

    // Set projection matrix data
    gl.uniformMatrix3fv(textureShaderInfo.locations.uMatrix, false, matrixProjection);

    let primitiveType = gl.TRIANGLES;
    let offset = 0;
    let count = textureShaderNumberOfVertices;
    gl.drawArrays(primitiveType, offset, count);
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

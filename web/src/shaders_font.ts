import { Mat3Utils } from "./math";
import { initShaderProgram } from "./shaders";
import { WasmExports } from "./exports";

export interface FontShaderInfo {
	program: WebGLProgram,
	buffers: {
		aPosition: WebGLBuffer,
		aTexCoord: WebGLBuffer,
	},
	locations: {
		aPosition: number,
		aTexCoord: number,
		uMatrix: WebGLUniformLocation,
		uImage: WebGLUniformLocation,
	},
	textures: {
		font: WebGLTexture,
	}

}

export function fontShaderSetup(gl: WebGLRenderingContext): FontShaderInfo {
	let vertexShaderSource = `
      attribute vec2 aPosition;
      attribute vec2 aTexCoord;
      uniform mat3 uMatrix;
      varying vec2 vTexCoord;
      
      void main() {
        gl_Position = vec4((uMatrix * vec3(aPosition, 1)).xy, 0, 1);
        vTexCoord = aTexCoord;
      }
    `;

	let fragmentShaderSource = `
      precision mediump float;

      // This is never set in the code as it defaults to using 
      // texture unit 0
      uniform sampler2D uImage;
      varying vec2 vTexCoord;

      void main() {
        vec4 color = texture2D(uImage, vTexCoord);
	// discard black background for the font
	if (color.r < 0.01 && color.g < 0.01 && color.b < 0.01) {
	    discard;
	}
	gl_FragColor = vec4(color.rgb, 1.0);
      }
    `;

	let shaderProgram = initShaderProgram(gl, vertexShaderSource, fragmentShaderSource)!;

	let shaderInfo = {
		program: shaderProgram,
		buffers: {
			aPosition: gl.createBuffer()!,
			aTexCoord: gl.createBuffer()!,
		},
		locations: {
			aPosition: gl.getAttribLocation(shaderProgram, "aPosition"),
			aTexCoord: gl.getAttribLocation(shaderProgram, "aTexCoord"),
			uMatrix: gl.getUniformLocation(shaderProgram, "uMatrix")!,
			uImage: gl.getUniformLocation(shaderProgram, "uImage")!,
		},
		textures: {
			font: gl.createTexture()!,
		}
	};

	// Enable indices at vertex attributes to be interpreted as such
	gl.enableVertexAttribArray(shaderInfo.locations.aPosition);
	gl.enableVertexAttribArray(shaderInfo.locations.aTexCoord);

	// Enable some config to make alpha in images blend with the rest.
	gl.pixelStorei(gl.UNPACK_PREMULTIPLY_ALPHA_WEBGL, true);
	gl.enable(gl.BLEND);
	gl.blendFunc(gl.SRC_ALPHA, gl.ONE_MINUS_SRC_ALPHA);

	return shaderInfo;
}

export function fontShaderDrawFrame(
	gl: WebGLRenderingContext,
	fontShaderInfo: FontShaderInfo,
	wasm: WebAssembly.Instance & { exports: WasmExports },
	wasmMemory: WebAssembly.Memory,
) {
	gl.useProgram(fontShaderInfo.program);

	gl.activeTexture(gl.TEXTURE2);
	gl.bindTexture(gl.TEXTURE_2D, fontShaderInfo.textures.font);

	let numberOfTriangles = wasm.exports.fontShaderGetTrianglesCount();
	let numberOfVertices = numberOfTriangles * 3;
	let pointsPerVertex = 2;
	let bytesPerFloat32 = 4;

	gl.bindBuffer(gl.ARRAY_BUFFER, fontShaderInfo.buffers.aPosition);
	gl.vertexAttribPointer(
		fontShaderInfo.locations.aPosition,
		2,  // size: components per iteration
		gl.FLOAT,  // data type
		false, // normalize
		0, // stride: bytes between beggining of consecutive vertex attributes in buffer
		0 // offset: where to start reading data from the buffer
	);

	let aPositionBufferBase = wasm.exports.getBufferBase(4);
	let aPositionBufferEnd = aPositionBufferBase + numberOfVertices * pointsPerVertex * bytesPerFloat32;
	let aPositionSlice = wasmMemory.buffer.slice(aPositionBufferBase, aPositionBufferEnd);
	let aPositionValues = new Float32Array(aPositionSlice);
	gl.bufferData(gl.ARRAY_BUFFER, aPositionValues, gl.STATIC_DRAW);

	gl.bindBuffer(gl.ARRAY_BUFFER, fontShaderInfo.buffers.aTexCoord);
	gl.vertexAttribPointer(
		fontShaderInfo.locations.aTexCoord,
		2,  // size: components per iteration
		gl.FLOAT,  // data type
		false, // normalize
		0, // stride: bytes between beggining of consecutive vertex attributes in buffer
		0 // offset: where to start reading data from the buffer
	);

	let aTexCoordBufferBase = wasm.exports.getBufferBase(5);
	let aTexCoordBufferEnd = aTexCoordBufferBase + numberOfVertices * pointsPerVertex * bytesPerFloat32;
	let aTexCoordSlice = wasmMemory.buffer.slice(aTexCoordBufferBase, aTexCoordBufferEnd);
	let aTexCoordValues = new Float32Array(aTexCoordSlice);
	gl.bufferData(gl.ARRAY_BUFFER, aTexCoordValues, gl.STATIC_DRAW);

	gl.uniformMatrix3fv(fontShaderInfo.locations.uMatrix, false, Mat3Utils.projection(gl.canvas.width, gl.canvas.height));
	gl.uniform1i(fontShaderInfo.locations.uImage, 2);

	gl.drawArrays(gl.TRIANGLES, 0, numberOfVertices);
}

export function fontShaderSetTexture(gl: WebGLRenderingContext, glTargetTexture: string, shaderInfo: FontShaderInfo, image: HTMLImageElement) {
	// Make shader texture the active texture
	// Make the target texture the active gl texture
	gl.activeTexture(gl[glTargetTexture as keyof WebGLRenderingContext] as number);
	// Bind the font texture to TEXTURE_2D binding point
	gl.bindTexture(gl.TEXTURE_2D, shaderInfo.textures.font);

	// Set texture parameters
	gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE)
	gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE)
	gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST)
	gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST)

	// Upload font image to the GPU's texture object
	gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, gl.RGBA, gl.UNSIGNED_BYTE, image)
}

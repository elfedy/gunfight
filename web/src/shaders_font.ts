import { Mat3Utils } from "./math";
import { initShaderProgram } from "./shaders";

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
	if (color.r < 0.01 && color.g < 0.01 && color.b < 0.01)
	    discard;
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

// export function textureShaderSetTexture(gl: WebGLRenderingContext, glTargetTexture: string, shaderInfo: TextureShaderInfo, image: HTMLImageElement, name: string) {
// 	// Make shader texture the active texture
// 	// Make the target texture the active gl texture
// 	gl.activeTexture(gl[glTargetTexture as keyof WebGLRenderingContext] as number);
// 	// Bind the sprite texture to TEXTURE_2D binding point
// 	gl.bindTexture(gl.TEXTURE_2D, shaderInfo.textures[name as keyof typeof shaderInfo.textures]);
//
// 	// Set texture parameters
// 	gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE)
// 	gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE)
// 	gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST)
// 	gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST)
//
// 	// Upload sprite image to the GPU's texture object
// 	gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, gl.RGBA, gl.UNSIGNED_BYTE, image)
// }
//
export function fontShaderDrawFrame(gl: WebGLRenderingContext, fontShaderInfo: FontShaderInfo) {
	// FONT SHADER
	gl.useProgram(fontShaderInfo.program);

	// Ensure texture unit 2 is active and bound
	gl.activeTexture(gl.TEXTURE2);
	gl.bindTexture(gl.TEXTURE_2D, fontShaderInfo.textures.font);

	let positionVertices = [];
	for (let i = 0; i < 2; i++) {
		let pminx = 10 + i * 16;
		let pminy = 10;
		let pmaxx = pminx + 32;
		let pmaxy = pminy + 32;

		// Position vertices for a single quad covering the entire atlas
		positionVertices.push(
			pminx, pminy,    // bottom-left
			pminx, pmaxy,    // top-left
			pmaxx, pminy,    // bottom-right
			pminx, pmaxy,    // top-left
			pmaxx, pmaxy,    // top-right
			pmaxx, pminy     // bottom-right
		);

	}

	// Provide position coordinates
	gl.bindBuffer(gl.ARRAY_BUFFER, fontShaderInfo.buffers.aPosition);
	gl.vertexAttribPointer(
		fontShaderInfo.locations.aPosition,
		2,  // size: components per iteration
		gl.FLOAT,  // data type
		false, // normalize
		0, // stride: bytes between beggining of consecutive vertex attributes in buffer
		0 // offset: where to start reading data from the buffer
	);

	let aPositionValues = new Float32Array(positionVertices);
	gl.bufferData(gl.ARRAY_BUFFER, aPositionValues, gl.STATIC_DRAW);

	// Provide texture coordinates - use the entire texture (0,0) to (1,1)
	gl.bindBuffer(gl.ARRAY_BUFFER, fontShaderInfo.buffers.aTexCoord);
	gl.vertexAttribPointer(
		fontShaderInfo.locations.aTexCoord,
		2,  // size: components per iteration
		gl.FLOAT,  // data type
		false, // normalize
		0, // stride: bytes between beggining of consecutive vertex attributes in buffer
		0 // offset: where to start reading data from the buffer
	);

	let aWidth = 512;
	let aHeight = 192;

	let h = {
		x: 224,
		y: 64,
		w: 32,
		h: 32
	};

	let e = {
		x: 128,
		y: 64,
		w: 32,
		h: 32
	};

	let textureCoords: number[] = [];
	[h, e].forEach(char => {
		let minX = char.x / aWidth;
		let maxX = (char.x + char.w) / aWidth;
		let minY = (aHeight - char.y - char.h) / aHeight;
		let maxY = (aHeight - char.y) / aHeight;

		// Texture coordinates for the entire atlas
		textureCoords.push(
			minX, minY,    // bottom-left
			minX, maxY,    // top-left
			maxX, minY,    // bottom-right
			minX, maxY,    // top-left
			maxX, maxY,    // top-right
			maxX, minY     // bottom-right
		);
	})

	let aTexCoordValues = new Float32Array(textureCoords);
	gl.bufferData(gl.ARRAY_BUFFER, aTexCoordValues, gl.STATIC_DRAW);

	// Set projection matrix data
	gl.uniformMatrix3fv(fontShaderInfo.locations.uMatrix, false, Mat3Utils.projection(gl.canvas.width, gl.canvas.height));

	// Use texture unit 2 for the font atlas
	gl.uniform1i(fontShaderInfo.locations.uImage, 2);

	let primitiveType = gl.TRIANGLES;
	let offset = 0;
	let count = 12; // 6 vertices for 2 triangles
	gl.drawArrays(primitiveType, offset, count);

	// Debug: Check for WebGL errors
	let error = gl.getError();
	if (error !== gl.NO_ERROR) {
		console.error("WebGL error in font rendering:", error);
	}
}

export function fontShaderSetTexture(gl: WebGLRenderingContext, glTargetTexture: string, shaderInfo: FontShaderInfo, image: HTMLImageElement) {
	// Make shader texture the active texture
	// Make the target texture the active gl texture
	gl.activeTexture(gl[glTargetTexture as keyof WebGLRenderingContext] as number);
	// Bind the sprite texture to TEXTURE_2D binding point
	gl.bindTexture(gl.TEXTURE_2D, shaderInfo.textures.font);

	// Set texture parameters
	gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE)
	gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE)
	gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST)
	gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST)

	// Upload sprite image to the GPU's texture object
	gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, gl.RGBA, gl.UNSIGNED_BYTE, image)
}

export interface WasmExports {
	/** Returns the start address of the heap */
	getHeapBase(): number; // u8* → numeric pointer (i32)

	/** Returns a pointer to a buffer by index */
	getBufferBase(index: number): number; // u8* → numeric pointer (i32)

	/** Returns number of triangles for the color shader */
	colorShaderGetTrianglesCount(): number; // u32

	/** Returns number of triangles for the texture shader */
	textureShaderGetTrianglesCount(): number; // u32

	/** Returns number of triangles for the font shader */
	fontShaderGetTrianglesCount(): number; // u32

	/** Handles controller input events */
	processControllerInput(keyIndex: number, isDown: number): void; // (u32, bool32)

	/** Main frame update and render, takes DOMHighResTimeStamp */
	updateAndRender(timestamp: DOMHighResTimeStamp): void; // f64
}

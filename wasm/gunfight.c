#include "gunfight.h"

// NOTE(fede): We define everything statically here because we already have all the memory
// we will use (it is allocated and passed from js as a WebAssembly.Memory object.
// Defining variables used globally in the environment seems not worth it here
// as we would need to decode memory from js to wasm

// GLOBAL GAME STATE
global_variable bool32 globalIsInitialized = 0;
global_variable f64 globalLastTimestamp;

global_variable GameState globalGameState;

global_variable GameControllerInput globalGameControllerInputCurrent = {};
global_variable GameControllerInput globalGameControllerInputLastFrame = {};

global_variable u32 globalColorShaderTrianglesCount = 0;
global_variable u32 globalTextureShaderTrianglesCount = 0;

// GLOBAL BUFFERS


// Define indices for each buffer
enum bufferIndex {INDEX_COLOR_SHADER_VERTICES, INDEX_COLOR_SHADER_COLORS, INDEX_TEXTURE_SHADER_VERTICES};

global_variable u32 globalBufferSizes[3] = {
    // ColorShaderVertices: 4 f32 (4 bytes) per vertex, 3 vertices per triangle, 100 triangles
    4800,
    // ColorShaderColor: 4 f32 (4 bytes) per color, 100 triangles
    1600,
    // TextureShaderVertices: 4 f32 (4 bytes) per vertex, 3 vertices per triangle, 100 triangles
    4800
};


// GLOBAL FRAME STATE
export u8 *getHeapBase() {
    return __heap_base;
}

export u8 *getBufferBase(index) {
    u8 *ret = __heap_base;

    assert(index < arrayLength(globalBufferSizes));

    for(int i = 0; i < index; i++) {
        ret = ret + globalBufferSizes[i];
    }
    return ret;
}

export u32 colorShaderGetTrianglesCount() {
    return globalColorShaderTrianglesCount;
}

internal
void bufferPushf32(Buffer *buffer, f32 value) {
    *((f32 *)(buffer->current + buffer->offset)) = value;
    buffer->offset += sizeof(f32);
}

ColorShaderFrame colorShaderFrameInit() {
    ColorShaderFrame ret = {};
    ret.trianglesCount = 0;

    Buffer verticesBuffer = {};
    verticesBuffer.current = getBufferBase(INDEX_COLOR_SHADER_VERTICES);
    verticesBuffer.offset = 0;

    ret.verticesBuffer = verticesBuffer;

    Buffer colorsBuffer = {};
    colorsBuffer.current = getBufferBase(INDEX_COLOR_SHADER_COLORS);
    colorsBuffer.offset = 0;

    ret.colorsBuffer = colorsBuffer;

    return ret;
}

TextureShaderFrame textureShaderFrameInit() {
    TextureShaderFrame ret = {};
    ret.trianglesCount = 0;

    Buffer verticesBuffer = {};
    verticesBuffer.current = getBufferBase(INDEX_COLOR_SHADER_VERTICES);
    verticesBuffer.offset = 0;

    ret.verticesBuffer = verticesBuffer;

    return ret;
}

// CONTROLLER
export void processControllerInput(u32 keyIndex, bool32 isDown) {
    GameButtonState *buttonState = &globalGameControllerInputCurrent.buttons[keyIndex];
    buttonState->isDown = isDown;
}

// DRAW
internal void drawRectangle(
        ColorShaderFrame *colorShaderFrame,
        Color color,
        f32 minX,
        f32 minY,
        f32 maxX,
        f32 maxY
) {
  f32 vertices[12] = {
    minX, minY,
    minX, maxY,
    maxX, maxY,
    minX, minY,
    maxX, minY,
    maxX, maxY,
  };

  Buffer *verticesBuffer = &colorShaderFrame->verticesBuffer;
  Buffer *colorsBuffer = &colorShaderFrame->colorsBuffer;

  for(int i = 0; i < arrayLength(vertices); i++) {
    bufferPushf32(verticesBuffer, vertices[i]);
  }

  // every three vertices pairs (triangle) we need to specify color and increment triangles
  // count
  for(int i = 0; i < arrayLength(vertices)/6; i++) {
    bufferPushf32(colorsBuffer, color.r);
    bufferPushf32(colorsBuffer, color.g);
    bufferPushf32(colorsBuffer, color.b);
    bufferPushf32(colorsBuffer, color.a);

    colorShaderFrame->trianglesCount++;
  }
}


// RENDER

export void updateAndRender(f64 timestamp) {
  //TODO(fede): This values should be more dynamic. Also how to achieve this while
  // preserving aspect ratio?
  f32 levelHeightInPixels = 960;
  f32 levelWidthInPixels = 540;

  f32 playerHeightInMeters = 1.6f;
  f32 playerWidthInMeters = 1.6f;

  f32 levelHeightInMeters = 13.5f;
  f32 levelWidthInMeters = 24.0f;

  f32 metersToPixels = 40;

  f32 playerWidthInPixels = playerWidthInMeters * metersToPixels;
  f32 playerHeightInPixels = playerHeightInMeters * metersToPixels;

  if(!globalIsInitialized) {
    globalGameState.playerPosition.x = 1.0f;
    globalGameState.playerPosition.y = 5.0f;
    globalLastTimestamp = timestamp;
    globalIsInitialized = 1;
  }

  f32 vx = 0.0f; // meters per second
  f32 vy = 0.0f;

  if(globalGameControllerInputCurrent.moveUp.isDown) {
      vy = 1.5f;
  }
  if(globalGameControllerInputCurrent.moveDown.isDown) {
      vy = -1.5f;
  }
  if(globalGameControllerInputCurrent.moveRight.isDown) {
      vx = 1.5f;
  }
  if(globalGameControllerInputCurrent.moveLeft.isDown) {
      vx = -1.5f;
  }

  f32 dt = (f32)((timestamp - globalLastTimestamp) / 1000.0f); // in seconds
  f32 dx = vx * dt;
  f32 dy = vy * dt;
  globalGameState.playerPosition.x += dx;
  globalGameState.playerPosition.y += dy;

  // Collision with level boundaries
  f32 playerMaxX = levelWidthInMeters - playerWidthInMeters;
  f32 playerMinX = 0;
  f32 playerMaxY = levelHeightInMeters - playerHeightInMeters;
  f32 playerMinY = 0;
  if(globalGameState.playerPosition.x > playerMaxX) {
      globalGameState.playerPosition.x = playerMaxX;
  }
  if(globalGameState.playerPosition.x < playerMinX) {
      globalGameState.playerPosition.x = playerMinX;
  }
  if(globalGameState.playerPosition.y > playerMaxY) {
      globalGameState.playerPosition.y = playerMaxY;
  }
  if(globalGameState.playerPosition.y < playerMinY) {
      globalGameState.playerPosition.y = playerMinY;
  }

  //TODO: initialize shader frames
  ColorShaderFrame colorShaderFrame = colorShaderFrameInit();

  Position positionInPixels;
  positionInPixels.x = globalGameState.playerPosition.x * metersToPixels;
  positionInPixels.y = globalGameState.playerPosition.y * metersToPixels;

  f32 minX = positionInPixels.x;
  f32 minY = positionInPixels.y;
  f32 maxX = positionInPixels.x + playerWidthInPixels;
  f32 maxY = positionInPixels.y + playerHeightInPixels;

  Color color;
  color.r = 0.0f;
  color.g = 1.0f;
  color.b = 1.0f;
  color.a = 1.0f;

  drawRectangle(
      &colorShaderFrame,
      color,
      minX,
      minY,
      maxX,
      maxY
  );


  globalColorShaderTrianglesCount = colorShaderFrame.trianglesCount; 
  globalLastTimestamp = timestamp;
  globalGameControllerInputLastFrame = globalGameControllerInputCurrent;
}

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

global_variable u32 globalColorShaderFrameTrianglesCount = 0;
global_variable u32 globalTextureShaderFrameTrianglesCount = 0;

// GLOBAL BUFFERS


// Define indices for each buffer
enum bufferIndex {
    // Color shader vertex positions to be drawn next frame
    INDEX_COLOR_SHADER_A_POSITION,
    // Color shader uniform attrbute color to be set when drawing each triangle
    INDEX_COLOR_SHADER_U_COLOR,
    // Texture shader vertex postions to be drawn next frame
    INDEX_TEXTURE_SHADER_A_POSITION,
    // Texture shader texture coordinates for each vertex in aPosition attribute
    INDEX_TEXTURE_SHADER_A_TEX_COORD
};

global_variable u32 globalBufferSizes[4] = {
    // ColorShaderAPosition: 2 f32 (4 bytes) per vertex, 3 vertices per triangle, 100 triangles
    2400,
    // ColorShaderColor: 4 f32 (4 bytes) per color, 100 triangles
    1600,
    // TextureShaderAPosition: 2 f32 (4 bytes) per vertex, 3 vertices per triangle, 100 triangles
    2400,
    // Texture Shader ATexCoord: same as A position as we want a set of coordinates per vertex
    2400
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
    return globalColorShaderFrameTrianglesCount;
}

export u32 textureShaderGetTrianglesCount() {
    return globalTextureShaderFrameTrianglesCount;
}

internal
void bufferPushf32(Buffer *buffer, f32 value) {
    *((f32 *)(buffer->current + buffer->offset)) = value;
    buffer->offset += sizeof(f32);
}

// SHADER FRAMES
ColorShaderFrame colorShaderFrameInit() {
    ColorShaderFrame ret = {};
    ret.trianglesCount = 0;

    Buffer aPositionBuffer = {};
    aPositionBuffer.current = getBufferBase(INDEX_COLOR_SHADER_A_POSITION);
    aPositionBuffer.offset = 0;

    ret.aPositionBuffer = aPositionBuffer;

    Buffer uColorsBuffer = {};
    uColorsBuffer.current = getBufferBase(INDEX_COLOR_SHADER_U_COLOR);
    uColorsBuffer.offset = 0;

    ret.uColorsBuffer = uColorsBuffer;

    return ret;
}

TextureShaderFrame textureShaderFrameInit() {
    TextureShaderFrame ret = {};
    ret.trianglesCount = 0;

    Buffer aPositionBuffer = {};
    aPositionBuffer.current = getBufferBase(INDEX_TEXTURE_SHADER_A_POSITION);
    aPositionBuffer.offset = 0;

    Buffer aTexCoordBuffer = {};

    aTexCoordBuffer.current = getBufferBase(INDEX_TEXTURE_SHADER_A_TEX_COORD);
    aTexCoordBuffer.offset = 0;

    ret.aPositionBuffer = aPositionBuffer;
    ret.aTexCoordBuffer = aTexCoordBuffer;

    return ret;
}

// CONTROLLER
export void processControllerInput(u32 keyIndex, bool32 isDown) {
    GameButtonState *buttonState = &globalGameControllerInputCurrent.buttons[keyIndex];
    buttonState->isDown = isDown;
}

// DRAW
internal void colorShaderDrawRectangle(
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

  Buffer *aPositionBuffer = &colorShaderFrame->aPositionBuffer;
  Buffer *uColorsBuffer = &colorShaderFrame->uColorsBuffer;

  for(int i = 0; i < arrayLength(vertices); i++) {
    bufferPushf32(aPositionBuffer, vertices[i]);
  }

  // every three vertices pairs (triangle) we need to specify color and increment triangles
  // count
  for(int i = 0; i < arrayLength(vertices)/6; i++) {
    bufferPushf32(uColorsBuffer, color.r);
    bufferPushf32(uColorsBuffer, color.g);
    bufferPushf32(uColorsBuffer, color.b);
    bufferPushf32(uColorsBuffer, color.a);

    colorShaderFrame->trianglesCount++;
  }
}

internal void textureShaderDrawPlayer(
    TextureShaderFrame *textureShaderFrame,
    f32 minX,
    f32 minY,
    f32 maxX,
    f32 maxY
) {
  f32 aPositionVals[12] = {
    minX, minY,
    maxX, minY,
    minX, maxY,
    minX, maxY,
    maxX, minY,
    maxX, maxY,
  };

  f32 aTexCoordVals[12] = {
      0.0f,  0.0f,
      1.0f,  0.0f,
      0.0f,  1.0f,
      0.0f,  1.0f,
      1.0f,  0.0f,
      1.0f,  1.0f
  };

  Buffer *aPositionBuffer = &textureShaderFrame->aPositionBuffer;
  Buffer *aTexCoordBuffer = &textureShaderFrame->aTexCoordBuffer;

  for(int i = 0; i < arrayLength(aPositionVals); i++) {
    bufferPushf32(aPositionBuffer, aPositionVals[i]);
  }

  for(int i = 0; i < arrayLength(aTexCoordVals); i++) {
    bufferPushf32(aTexCoordBuffer, aTexCoordVals[i]);
  }

  textureShaderFrame->trianglesCount += 2;
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

  f32 dPlayerP = 5.0f;

  if(globalGameControllerInputCurrent.moveUp.isDown) {
      vy = dPlayerP;
  }
  if(globalGameControllerInputCurrent.moveDown.isDown) {
      vy = -dPlayerP;
  }
  if(globalGameControllerInputCurrent.moveRight.isDown) {
      vx = dPlayerP;
  }
  if(globalGameControllerInputCurrent.moveLeft.isDown) {
      vx = -dPlayerP;
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
  TextureShaderFrame textureShaderFrame = textureShaderFrameInit();

  Position positionInPixels;
  positionInPixels.x = globalGameState.playerPosition.x * metersToPixels;
  positionInPixels.y = globalGameState.playerPosition.y * metersToPixels;

  f32 minX = positionInPixels.x;
  f32 minY = positionInPixels.y;
  f32 maxX = positionInPixels.x + playerWidthInPixels;
  f32 maxY = positionInPixels.y + playerHeightInPixels;

  textureShaderDrawPlayer(&textureShaderFrame, minX, minY, maxX, maxY);

  globalColorShaderFrameTrianglesCount = colorShaderFrame.trianglesCount; 
  globalTextureShaderFrameTrianglesCount = textureShaderFrame.trianglesCount; 

  globalLastTimestamp = timestamp;
  globalGameControllerInputLastFrame = globalGameControllerInputCurrent;
}

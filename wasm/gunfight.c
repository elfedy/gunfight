#include "gunfight.h"

// ENV IMPORTS
void logFloat32(f32 val);

extern u8 *__heap_base;

// NOTE(fede): We define everything statically here because we already have all the memory
// we will use (it is allocated and passed from js as a WebAssembly.Memory object.
// Defining variables used globally in the environment seems not worth it here
// as we would need to decode memory from js to wasm

// GLOBAL GAME STATE
global_variable bool32 globalIsInitialized = 0;
global_variable f64 globalLastTimestamp;
global_variable GameState globalGameState;
global_variable GameControllerInput globalGameControllerInput = {};
global_variable GameControllerInput globalLastFrameControllerInput = {};

// GLOBAL BUFFERS
// 4 f32 (4 bytes) per vertex, 3 vertices per triangle, 100 triangles
// 4 * 4 * 3 * 100 = 4800 bytes =~ 4.6KB
global_variable u32 globalVertexBufferSize = 4800;
// 4 f32 (4 bytes) per color, 100 triangles
// 4 * 4 * 100 = 1600 bytes =~ 1.6KB
global_variable u32 globalColorBufferSize = 1600;

// GLOBAL FRAME STATE
global_variable u32 globalTrianglesCount = 0;

export u8 *getBufferBase() {
    return __heap_base;
}

// CONTROLLER
export void processControllerInput(u32 keyIndex, bool32 isDown) {
    GameButtonState *buttonState = &globalGameControllerInput.buttons[keyIndex];
    buttonState->isDown = isDown;
}

// DRAW
internal void drawRectangle(
        Buffer *vertexBuffer,
        Buffer *colorBuffer,
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

  f32 *vertexPointer = (f32 *)vertexBuffer->current;
  f32 *colorPointer = (f32 *)colorBuffer->current;

  for(int i = 0; i < arrayLength(vertices); i++) {
    *vertexPointer++ = vertices[i];
  }

  // every three vertices pairs (triangle) we need to specify color and increment triangles
  // count
  for(int i = 0; i < arrayLength(vertices)/6; i++) {
    *colorPointer++ = color.r;
    *colorPointer++ = color.g;
    *colorPointer++ = color.b;
    *colorPointer++ = color.a;

    ++globalTrianglesCount;
  }

  vertexBuffer->current = (u8 *)vertexPointer;
  colorBuffer->current = (u8 *)colorPointer;


}


// NOTE(fede)
// from base we store in order:
// 1) vertices
// 2) colors
export u8 *getVertexBufferBase() {
    return __heap_base;
}

export u8 *getColorBufferBase() {
    return (__heap_base + globalVertexBufferSize);
}

export u32 getTrianglesCount() {
    return globalTrianglesCount;
}

// RENDER
internal void resetFrameState() {
    globalTrianglesCount = 0;
}

export void updateAndRender(f64 timestamp) {
  resetFrameState();
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

  if(globalGameControllerInput.moveUp.isDown) {
      vy = 1.5f;
  }
  if(globalGameControllerInput.moveDown.isDown) {
      vy = -1.5f;
  }
  if(globalGameControllerInput.moveRight.isDown) {
      vx = 1.5f;
  }
  if(globalGameControllerInput.moveLeft.isDown) {
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

  Buffer verticesBuffer = { getVertexBufferBase(), getVertexBufferBase() };
  Buffer colorsBuffer = { getColorBufferBase(), getColorBufferBase() };

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
      &verticesBuffer,
      &colorsBuffer,
      color,
      minX,
      minY,
      maxX,
      maxY
  );


  globalLastTimestamp = timestamp;
}

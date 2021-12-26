#include "gunfight.h"

// ENV IMPORTS
void logFloat32(f32 val);

extern u8 *__heap_base;

// GLOBAL VARIABLES
global_variable bool32 globalIsInitialized = 0;
global_variable f64 globalLastTimestamp;

global_variable Position globalPlayerPositionInMeters;

// 4 f32 (4 bytes) per vertex, 3 vertices per triangle, 100 triangles
// 4 * 4 * 3 * 100 = 4800 bytes =~ 4.6KB
global_variable u32 globalVertexBufferSize = 4800;
// 4 f32 (4 bytes) per color, 100 triangles
// 4 * 4 * 100 = 1600 bytes =~ 1.6KB
global_variable u32 globalColorBufferSize = 1600;

global_variable u32 globalTrianglesCount = 0;

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

export u8 *getBufferBase() {
    return __heap_base;
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

  f32 playerHeightInMeters = 1.8f;
  f32 playerWidthInMeters = 0.6f;

  f32 metersToPixels = 40;

  f32 playerWidthInPixels = playerWidthInMeters * metersToPixels;
  f32 playerHeightInPixels = playerHeightInMeters * metersToPixels;

  if(!globalIsInitialized) {
    globalPlayerPositionInMeters.x = 1.0f;
    globalPlayerPositionInMeters.y = 5.0f;
    globalLastTimestamp = timestamp;
    globalIsInitialized = 1;
  }

  f32 vx = 1.5f; // meters per second
  f32 dt = (f32)((timestamp - globalLastTimestamp) / 1000.0f); // in seconds
  f32 dx = vx * dt;
  globalPlayerPositionInMeters.x += dx;

  Buffer verticesBuffer = { getVertexBufferBase(), getVertexBufferBase() };
  Buffer colorsBuffer = { getColorBufferBase(), getColorBufferBase() };

  Position positionInPixels;
  positionInPixels.x = globalPlayerPositionInMeters.x * metersToPixels;
  positionInPixels.y = globalPlayerPositionInMeters.y * metersToPixels;

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

  Color anotherColor;
  anotherColor.r = 1.0f;
  anotherColor.g = 1.0f;
  anotherColor.b = 0.0f;
  anotherColor.a = 1.0f;

  drawRectangle(
      &verticesBuffer,
      &colorsBuffer,
      anotherColor,
      100,
      100,
      200,
      200 
  );

  anotherColor.r = 1.0f;
  anotherColor.g = 0.0f;
  anotherColor.b = 1.0f;
  anotherColor.a = 1.0f;

  drawRectangle(
      &verticesBuffer,
      &colorsBuffer,
      anotherColor,
      400,
      400,
      450,
      450 
  );

  globalLastTimestamp = timestamp;
}

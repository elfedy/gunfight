typedef unsigned char u8;
typedef unsigned long u32;
typedef long i32;

typedef float f32;
typedef double f64;

typedef long bool32;

#define internal static
#define global_variable static

#define export __attribute__((visibility("default")))

#define arrayLength(A) sizeof(A)/sizeof(A[0])

#define kilobytes(value) ((value)*1024LL)
#define megabytes(value) (kilobytes(value)*1024LL)
#define gigabytes(value) (megabytes(value)*1024LL)

// ENV IMPORTS
void logFloat32(f32 val);

extern u8 __heap_base;

internal void drawRectangle(f32 *verticesBuffer, f32 minX, f32 minY, f32 maxX, f32 maxY) {
  f32 vertices[12] = {
    minX, minY,
    minX, maxY,
    maxX, maxY,
    minX, minY,
    maxX, minY,
    maxX, maxY,
  };

  for(int i = 0; i < arrayLength(vertices); i++) {
    *verticesBuffer++ = vertices[i];
  }
}

typedef struct Position {
  f32 x;
  f32 y;
} Position;

global_variable Position POSITION_IN_METERS;
global_variable bool32 IS_INITIALIZED = 0;
global_variable f64 LAST_TIMESTAMP;

export u8 *getBufferBase() {
    return &__heap_base;
}

export void updateAndRender(f64 timestamp) {
  //TODO(fede): This values should be more dynamic. Also how to achieve this while
  // preserving aspect ratio?
  f32 levelHeightInPixels = 960;
  f32 levelWidthInPixels = 540;

  f32 playerHeightInMeters = 1.8f;
  f32 playerWidthInMeters = 0.6f;

  f32 metersToPixels = 40;

  f32 playerWidthInPixels = playerWidthInMeters * metersToPixels;
  f32 playerHeightInPixels = playerHeightInMeters * metersToPixels;

  if(!IS_INITIALIZED) {
    POSITION_IN_METERS.x = 1.0f;
    POSITION_IN_METERS.y = 5.0f;
    LAST_TIMESTAMP = timestamp;
    IS_INITIALIZED = 1;
  }

  f32 vx = 0.05f; // meters per second
  f32 dt = (f32)((timestamp - LAST_TIMESTAMP) / 1000.0f); // in seconds
  f32 dx = vx * dt;
  POSITION_IN_METERS.x += dx;

  f32 *verticesBuffer = (f32 *)getBufferBase();

  Position positionInPixels;
  positionInPixels.x = POSITION_IN_METERS.x * metersToPixels;
  positionInPixels.y = POSITION_IN_METERS.y * metersToPixels;

  f32 minX = positionInPixels.x;
  f32 minY = positionInPixels.y;
  f32 maxX = positionInPixels.x + playerWidthInPixels;
  f32 maxY = positionInPixels.y + playerHeightInPixels;

  drawRectangle(
      verticesBuffer,
      minX,
      minY,
      maxX,
      maxY
  );
}

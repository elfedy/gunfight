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

#define assert(expression) if(!(expression)) { __builtin_trap(); }

#include "gunfight_imports.h"


// GAME STATE 
typedef struct Position {
  f32 x;
  f32 y;
} Position;

typedef struct GameState {
  Position playerPosition;
} GameState;


// CONTROLLER
typedef struct GameButtonState
{
  bool32 isDown;
} GameButtonState;

typedef struct GameControllerInput
{
  union {
    GameButtonState buttons[6];
    struct {
      GameButtonState moveUp;
      GameButtonState moveDown;
      GameButtonState moveRight;
      GameButtonState moveLeft;

      GameButtonState action;
      GameButtonState pause;
    };
  };
} GameControllerInput;

// BUFFERS

typedef struct Buffer {
  u8 *current;
  u32 offset;
} Buffer;

// RENDERING

// Data about what the color shader should draw on the next frame
typedef struct ColorShaderFrame {
  u32 trianglesCount;
  Buffer verticesBuffer;
  Buffer colorsBuffer;
} ColorShaderFrame;

// Data for the texture shader to draw on the current frame
typedef struct TextureShaderFrame {
  u32 trianglesCount;
  Buffer verticesBuffer;
} TextureShaderFrame;

typedef struct Color {
  f32 r;
  f32 g;
  f32 b;
  f32 a;
} Color;
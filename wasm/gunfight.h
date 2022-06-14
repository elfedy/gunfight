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

#define seconds(value) (value * 1000)

#define assert(expression) if(!(expression)) { __builtin_trap(); }

#include "gunfight_imports.h"
#include "gunfight_math.h"

// ENTITIES
struct Bullet {
  V2 p;
  V2 dP;
  bool32 firing;
};

struct Enemy {
  V2 p;
  V2 dP;
  bool32 hasSpawned;
};

// GAME STATE 
struct GameState {
  V2 playerP;
  V2 dPlayerP;

  Bullet playerBullets[3];
  

  f64 enemyLastSpawned;
  u32 enemiesIndex;
  Enemy enemies[20];

  u32 playerBulletCount;
  f64 playerBulletLastFired;
}; 

// CONTROLLER
struct GameButtonState
{
  bool32 isDown;
};

struct GameControllerInput
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
};

// BUFFERS

struct Buffer {
  u8 *current;
  u32 offset;
};

// RENDERING
struct Color {
  f32 r;
  f32 g;
  f32 b;
  f32 a;
};

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

// ENUMS
enum textureIndex {
  PLAYER,
  ENEMY_SHOOTER
};

// ENTITIES
struct Bullet {
  V2 p;
  V2 dP;
  bool32 firing = false;
};

struct Enemy {
  V2 p;
  V2 dP;
  V2 intendedDirection;
  bool32 active = false;

  f64 bulletLastFired;
  Bullet bullets[3];
};

// GAME STATE 
struct GameState {
  f64 enemyLastSpawned;
  u32 enemiesIndex;
  u32 enemiesCurrentCount;
  Enemy enemies[10];

  V2 playerP;
  V2 dPlayerP;
  bool32 playerIsInvulnerable;
  f64 playerInvulnerableSince;
  Bullet playerBullets[3];
  u32 playerLives;
  
  f64 playerAnimationInvulnerableLastTransition;
  bool32 playerAnimationInvulnerableIsShowing;
  
  bool32 gameOver;

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

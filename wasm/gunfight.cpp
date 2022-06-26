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
extern "C"
export u8 *getHeapBase() {
    return __heap_base;
}

extern "C"
export u8 *getBufferBase(int index) {
    u8 *ret = __heap_base;

    assert(index < arrayLength(globalBufferSizes));

    for(int i = 0; i < index; i++) {
        ret = ret + globalBufferSizes[i];
    }
    return ret;
}

extern "C"
export u32 colorShaderGetTrianglesCount() {
    return globalColorShaderFrameTrianglesCount;
}

extern "C"
export u32 textureShaderGetTrianglesCount() {
    return globalTextureShaderFrameTrianglesCount;
}

internal
void bufferPushF32(Buffer *buffer, f32 value) {
    *((f32 *)(buffer->current + buffer->offset)) = value;
    buffer->offset += sizeof(f32);
}

#include "gunfight_shaders.h"


// CONTROLLER
extern "C"
export void processControllerInput(u32 keyIndex, bool32 isDown) {
    GameButtonState *buttonState = &globalGameControllerInputCurrent.buttons[keyIndex];
    buttonState->isDown = isDown;
}

// RENDER

extern "C"
export void updateAndRender(f64 timestamp) {
  //TODO(fede): This values should be more dynamic. Also how to achieve this while
  // preserving aspect ratio?
  f32 playerHeightInMeters = 1.6f;
  f32 playerWidthInMeters = 1.6f;

  f32 enemyHeightInMeters = 1.6f;
  f32 enemyWidthInMeters = 1.6f;

  f32 levelHeightInMeters = 13.5f;
  f32 levelWidthInMeters = 24.0f;

  f32 metersToPixels = 40;

  f32 playerWidthInPixels = playerWidthInMeters * metersToPixels;
  f32 playerHeightInPixels = playerHeightInMeters * metersToPixels;

  f32 enemyWidthInPixels = enemyWidthInMeters * metersToPixels;
  f32 enemyHeightInPixels = enemyHeightInMeters * metersToPixels;

  if(!globalIsInitialized) {
    globalGameState.playerP = {1.0f, 5.0f};
    globalGameState.dPlayerP = {};
    globalGameState.enemiesIndex = 0;
    globalGameState.enemiesCurrentCount = 0;
    globalGameState.enemyLastSpawned = timestamp;

    // clear all bullets to not firing
    for(int i = 0; i < arrayLength(globalGameState.playerBullets); ++i) {
      globalGameState.playerBullets[i] = {
        {50, 50},
        {0, 0},
        0
      };
    }
    
    // clear all enemies to not spawned
    for(int i = 0; i < arrayLength(globalGameState.enemies); ++i) {
      globalGameState.enemies[i] = {
        {0, 0},
        {0, 0},
        {0, 0},
        0
      };
    }

    globalLastTimestamp = timestamp;
    globalIsInitialized = 1;
  }

  // Initialize Shader Frames
  ColorShaderFrame colorShaderFrame = colorShaderFrameInit();
  TextureShaderFrame textureShaderFrame = textureShaderFrameInit();

  f32 dt = (f32)((timestamp - globalLastTimestamp) / 1000.0f); // in seconds

  // Compute Player Movement
  V2 ddPlayerP = {0.0f, 0.0f};

  if(globalGameControllerInputCurrent.moveUp.isDown) {
      ddPlayerP.y += 1.0f;
  }
  if(globalGameControllerInputCurrent.moveDown.isDown) {
      ddPlayerP.y = -1.0f;
  }
  if(globalGameControllerInputCurrent.moveRight.isDown) {
      ddPlayerP.x += 1.0f;
  }
  if(globalGameControllerInputCurrent.moveLeft.isDown) {
      ddPlayerP.x -= 1.0f;
  }

  if((ddPlayerP.x != 0.0f) && (ddPlayerP.y != 0.0f))
  {
    ddPlayerP *= 0.707186781197f;
  }

  f32 playerBaseAcceleration = 20.0f;
  ddPlayerP *= playerBaseAcceleration;
  // Add "Friction"
  ddPlayerP += -2.5f*globalGameState.dPlayerP;


  V2 newPlayerP = computeNewPosition(globalGameState.playerP, globalGameState.dPlayerP, ddPlayerP, dt);
  V2 newDPlayerP = computeNewVelocity(globalGameState.dPlayerP, ddPlayerP, dt);

  // Collision with level boundaries
  f32 playerMaxX = levelWidthInMeters - playerWidthInMeters;
  f32 playerMinX = 0;
  f32 playerMaxY = levelHeightInMeters - playerHeightInMeters;
  f32 playerMinY = 0;
  if(newPlayerP.x > playerMaxX) {
      newPlayerP.x = playerMaxX;
      newDPlayerP = {0, 0};
  }
  if(globalGameState.playerP.x < playerMinX) {
      newPlayerP.x = playerMinX;
      newDPlayerP.x = 0;
  }
  if(globalGameState.playerP.y > playerMaxY) {
      newPlayerP.y = playerMaxY;
      newDPlayerP = {0, 0};
  }
  if(globalGameState.playerP.y < playerMinY) {
      newPlayerP.y = playerMinY;
      newDPlayerP = {0, 0};
  }

  globalGameState.playerP = newPlayerP;
  globalGameState.dPlayerP = newDPlayerP;

  V2 playerPInPixels = globalGameState.playerP * metersToPixels;

  V2 minPlayerRect = playerPInPixels;
  V2 maxPlayerRect = playerPInPixels + V2{playerWidthInPixels, playerHeightInPixels};

  textureShaderDrawTexture(&textureShaderFrame, PLAYER, minPlayerRect, maxPlayerRect);

  // ENEMIES
  // Update Enemies
  f32 enemyBaseAcceleration = 20.0f;
  f32 enemyMinX = 0.3f * levelWidthInMeters;
  f32 enemyMaxX = levelWidthInMeters - enemyWidthInMeters * 1.2;

  for(int i = 0; i < arrayLength(globalGameState.enemies); ++i) {
    Enemy *currentEnemy = &globalGameState.enemies[i];

    if(currentEnemy->active) {
      // Compute Enemy Movement
      // Movement AI

      if(currentEnemy->p.x < enemyMinX) {
        currentEnemy->intendedDirection = {1, 0};
      } 
      if(currentEnemy->p.x > enemyMaxX) {
        currentEnemy->intendedDirection = {-1, 0};
      }
      V2 currentEnemyDdP = currentEnemy->intendedDirection * enemyBaseAcceleration;
      currentEnemyDdP += -2.5f*currentEnemy->dP;

      V2 newEnemyP = computeNewPosition(currentEnemy->p, currentEnemy->dP, currentEnemyDdP, dt);
      V2 newDEnemyP = computeNewVelocity(currentEnemy->dP, currentEnemyDdP, dt);

      currentEnemy->p = newEnemyP;
      currentEnemy->dP = newDEnemyP;
    }
  }

  // Update player bullets
  // TODO(fede): move these definitions somewhere else
  f32 bulletWidthInMeters = 0.2f; 
  f32 bulletHeightInMeters = 0.2f; 
  f32 bulletWidthInPixels = bulletWidthInMeters * metersToPixels;
  f32 bulletHeightInPixels = bulletHeightInMeters * metersToPixels;

  for(int i = 0; i < arrayLength(globalGameState.playerBullets); ++i) {
    Bullet *currentBullet = &globalGameState.playerBullets[i];

    if(currentBullet->firing) {
      // Compute Bullet Movement
      V2 ddBulletP = {0.0f, 0.0f};
      V2 newBulletP = computeNewPosition(currentBullet->p, currentBullet->dP, ddBulletP, dt);
      V2 newDBulletP = computeNewVelocity(currentBullet->dP, ddBulletP, dt);

      // Collision with enemies
      bool32 hasCollidedWithEnemy = false;
      // NOTE(fede): The leftmost enemy is the one that is considered that collided
      Enemy *collidedEnemy;

      for(int i = 0; i < arrayLength(globalGameState.enemies); i++) {
        Enemy *enemy = &globalGameState.enemies[i];
        if(enemy->active) {
          V2 enemyTopRight = 
            enemy->p + V2{enemyWidthInMeters, enemyHeightInMeters};
          V2 bulletTopRight =
            newBulletP + V2{bulletWidthInMeters, bulletHeightInMeters};
          bool32 collided = 
            rectanglesAreColliding(
                enemy->p, enemyTopRight, newBulletP, bulletTopRight);

          if(collided) {
            if(!hasCollidedWithEnemy) {
              hasCollidedWithEnemy = true;
              collidedEnemy = enemy;
            } else {
              if(collidedEnemy->p.x > enemy->p.x) {
                collidedEnemy = enemy;
              }
            }
          }
        }
      }

      if(hasCollidedWithEnemy) {
        currentBullet->firing = false;
        collidedEnemy->active = false;
        --globalGameState.enemiesCurrentCount;
      } else {
        // "Collision" with right wall
        if(newBulletP.x < levelWidthInMeters) {
          currentBullet->p = newBulletP;
          currentBullet->dP = newDBulletP;
        } else {
          currentBullet->firing = false;
        }
      }
    }
  }
  
  // Spawn Enemies
  bool32 enemyBufferFull = globalGameState.enemiesCurrentCount == arrayLength(globalGameState.enemies);

  if(!enemyBufferFull && timestamp - globalGameState.enemyLastSpawned > seconds(2)) {
    // Loop until we find a not active slot to spawn the enemy
    while(true) {
      Enemy *currentEnemy = &globalGameState.enemies[globalGameState.enemiesIndex];
      globalGameState.enemiesIndex++;
      if(globalGameState.enemiesIndex >= arrayLength(globalGameState.enemies)){
        globalGameState.enemiesIndex = 0;
      }

      if(!currentEnemy->active) {
        currentEnemy->active = true;
        f32 spawnHeight = (levelHeightInMeters - enemyHeightInMeters) * envRandF32();
        currentEnemy->p = { levelWidthInMeters - enemyWidthInMeters, spawnHeight };
        currentEnemy->dP = { -5.0f, 0 };
        currentEnemy->intendedDirection = { -1, 0};
        break;
      }
    }
    globalGameState.enemyLastSpawned = timestamp;
    ++globalGameState.enemiesCurrentCount;
  }

  // Render Enemies
  for(int i = 0; i < arrayLength(globalGameState.enemies); ++i) {
    Enemy *currentEnemy = &globalGameState.enemies[i];

    if(currentEnemy->active) {
        // render;
        Color color = {0.0f, 1.0f, 1.0f, 1.0f};
        V2 bottomLeft = currentEnemy->p * metersToPixels;
        V2 relTopRight = {enemyWidthInPixels, enemyWidthInPixels};
        V2 enemyTopRight = bottomLeft + relTopRight;

        textureShaderDrawTexture(&textureShaderFrame, ENEMY_SHOOTER, bottomLeft, enemyTopRight);
    }
  }


  // Fire new player Bullets
  bool32 actionButtonToggledDown = 
    globalGameControllerInputCurrent.action.isDown && !globalGameControllerInputLastFrame.action.isDown;

  if(actionButtonToggledDown) {
    for(int i = 0; i < arrayLength(globalGameState.playerBullets); i++) {
      Bullet *currentBullet = &globalGameState.playerBullets[i];
      if(!currentBullet->firing) {
        currentBullet->firing = 1;
        currentBullet->p = globalGameState.playerP + V2{playerWidthInMeters, playerHeightInMeters*0.375f - bulletWidthInMeters/2};
        currentBullet->dP = {30, 0};
        break;
      }
    }
  }

  // Render player bullets
  for(int i = 0; i < arrayLength(globalGameState.playerBullets); ++i) {
    Bullet *currentBullet = &globalGameState.playerBullets[i];

    if(currentBullet->firing) {
      if(currentBullet->p.x < levelWidthInMeters) {
        // render;
        Color color = {1.0f, 1.0f, 1.0f, 1};
        V2 min = currentBullet->p * metersToPixels;
        V2 relTopRight = {bulletWidthInPixels, bulletHeightInPixels};
        V2 bulletTopRight = min + relTopRight;

        colorShaderDrawRectangle(&colorShaderFrame, color, min, bulletTopRight);
      }
    }
  }

  // Update globals
  globalColorShaderFrameTrianglesCount = colorShaderFrame.trianglesCount; 
  globalTextureShaderFrameTrianglesCount = textureShaderFrame.trianglesCount; 

  globalLastTimestamp = timestamp;
  globalGameControllerInputLastFrame = globalGameControllerInputCurrent;
}

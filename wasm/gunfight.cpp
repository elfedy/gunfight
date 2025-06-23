#include "gunfight.h"

// NOTE(fede): We define everything statically here because we already have all
// the memory we will use (it is allocated and passed from js as a
// WebAssembly.Memory object. Defining variables used globally in the
// environment seems not worth it here as we would need to decode memory from js
// to wasm

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
    // ColorShaderAPosition: 2 f32 (4 bytes) per vertex, 3 vertices per
    // triangle, 150 triangles
    3600,
    // ColorShaderColor: 4 f32 (4 bytes) per color, 150 triangles
    2400,
    // TextureShaderAPosition: 2 f32 (4 bytes) per vertex, 3 vertices per
    // triangle, 150 triangles
    3600,
    // Texture Shader ATexCoord: same as A position as we want a set of
    // coordinates per vertex
    3600};

// GLOBAL FRAME STATE
extern "C" export u8 *getHeapBase() { return __heap_base; }

extern "C" export u8 *getBufferBase(int index) {
  u8 *ret = __heap_base;

  assert(index < arrayLength(globalBufferSizes));

  for (int i = 0; i < index; i++) {
    ret = ret + globalBufferSizes[i];
  }
  return ret;
}

extern "C" export u32 colorShaderGetTrianglesCount() {
  return globalColorShaderFrameTrianglesCount;
}

extern "C" export u32 textureShaderGetTrianglesCount() {
  return globalTextureShaderFrameTrianglesCount;
}

// TODO: This should throw an error on buffer overflow
// buffer should have A len
internal void bufferPushF32(Buffer *buffer, f32 value) {
  *((f32 *)(buffer->current + buffer->offset)) = value;
  buffer->offset += sizeof(f32);
}

#include "gunfight_shaders.h"

// GAME STATE
internal void playerStartInvulnerable(GameState *gameState, f64 timestamp) {
  gameState->playerIsInvulnerable = true;
  gameState->playerInvulnerableSince = timestamp;
}

internal void playerStopInvulnerable(GameState *gameState) {
  gameState->playerIsInvulnerable = false;
}

internal EnemyAIMode enemyPickAIMode() {
  f32 rand = envRandF32();
  // if(rand < 0.20) {
  if (1) {
    return ENEMY_AI_MOVING_UP;
  } else if (rand < 0.2) {
    return ENEMY_AI_MOVING_DOWN;
  } else if (rand < 0.6) {
    return ENEMY_AI_CHASING_PLAYER;
  } else {
    return ENEMY_AI_MOVING_HORIZONTAL;
  }
}

// CONTROLLER
extern "C" export void processControllerInput(u32 keyIndex, bool32 isDown) {
  GameButtonState *buttonState =
      &globalGameControllerInputCurrent.buttons[keyIndex];
  buttonState->isDown = isDown;
}

// RENDER
internal void endRenderFrame(ColorShaderFrame *colorShaderFrame,
                             TextureShaderFrame *textureShaderFrame) {
  globalColorShaderFrameTrianglesCount = colorShaderFrame->trianglesCount;
  globalTextureShaderFrameTrianglesCount = textureShaderFrame->trianglesCount;
}

internal void renderGameOver(ColorShaderFrame *colorShaderFrame,
                             TextureShaderFrame *textureShaderFrame,
                             f32 levelWidth, f32 levelHeight) {
  V2 min = {0, 0};
  V2 max = {levelWidth, levelHeight};
  Color color = {0.0f, 0.0f, 0.0f, 1.0f};
  colorShaderDrawRectangle(colorShaderFrame, color, min, max);
  endRenderFrame(colorShaderFrame, textureShaderFrame);
}

extern "C" export void updateAndRender(f64 timestamp) {
  // Dimensions
  f32 playerHeightInMeters = 1.6f;
  f32 playerWidthInMeters = 1.6f;

  f32 enemyHeightInMeters = 1.6f;
  f32 enemyWidthInMeters = 1.6f;

  // 16:9 aspect ratio
  // TODO: this should be dynamic
  f32 levelHeightInPixels = 540;
  f32 levelWidthInPixels = 960;

  f32 levelHeightInMeters = 13.5f;
  f32 levelWidthInMeters = 24.0f;

  f32 metersToPixels = levelHeightInPixels / levelHeightInMeters;

  f32 playerWidthInPixels = playerWidthInMeters * metersToPixels;
  f32 playerHeightInPixels = playerHeightInMeters * metersToPixels;

  f32 enemyWidthInPixels = enemyWidthInMeters * metersToPixels;
  f32 enemyHeightInPixels = enemyHeightInMeters * metersToPixels;

  f32 bulletWidthInMeters = 0.2f;
  f32 bulletHeightInMeters = 0.2f;
  f32 bulletWidthInPixels = bulletWidthInMeters * metersToPixels;
  f32 bulletHeightInPixels = bulletHeightInMeters * metersToPixels;

  if (!globalIsInitialized) {
    globalGameState.playerP = {1.0f, 5.0f};
    globalGameState.dPlayerP = {};
    globalGameState.playerLives = 3;

    globalGameState.enemiesIndex = 0;
    globalGameState.enemiesCurrentCount = 0;
    globalGameState.enemyNextSpawn = (timestamp + seconds(2));

    globalGameState.gameOver = false;

    globalLastTimestamp = timestamp;
    globalIsInitialized = 1;
  }

  // Initialize Shader Frames
  ColorShaderFrame colorShaderFrame = colorShaderFrameInit();
  TextureShaderFrame textureShaderFrame = textureShaderFrameInit();

  f32 dt = (f32)((timestamp - globalLastTimestamp) / 1000.0f); // in seconds

  if (globalGameState.gameOver) {
    renderGameOver(&colorShaderFrame, &textureShaderFrame, levelWidthInPixels,
                   levelHeightInPixels);
    return;
  }

  // Compute Player Movement
  V2 ddPlayerP = {0.0f, 0.0f};

  if (globalGameControllerInputCurrent.moveUp.isDown) {
    ddPlayerP.y += 1.0f;
  }
  if (globalGameControllerInputCurrent.moveDown.isDown) {
    ddPlayerP.y = -1.0f;
  }
  if (globalGameControllerInputCurrent.moveRight.isDown) {
    ddPlayerP.x += 1.0f;
  }
  if (globalGameControllerInputCurrent.moveLeft.isDown) {
    ddPlayerP.x -= 1.0f;
  }

  if ((ddPlayerP.x != 0.0f) && (ddPlayerP.y != 0.0f)) {
    ddPlayerP *= 0.707186781197f;
  }

  f32 playerBaseAcceleration = 20.0f;
  ddPlayerP *= playerBaseAcceleration;
  // Add "Friction"
  ddPlayerP += -2.5f * globalGameState.dPlayerP;

  V2 newPlayerP = computeNewPosition(globalGameState.playerP,
                                     globalGameState.dPlayerP, ddPlayerP, dt);
  V2 newDPlayerP = computeNewVelocity(globalGameState.dPlayerP, ddPlayerP, dt);

  // Collision with level boundaries
  f32 playerMaxX = levelWidthInMeters - playerWidthInMeters;
  f32 playerMinX = 0;
  f32 playerMaxY = levelHeightInMeters - playerHeightInMeters;
  f32 playerMinY = 0;

  CollisionResult boundariesCollision = getV2CollisionWithBoundaries(
      newPlayerP, newDPlayerP, V2{playerMinX, playerMinY},
      V2{playerMaxX, playerMaxY});

  newPlayerP = boundariesCollision.newPosition;
  newDPlayerP = boundariesCollision.newDPosition;

  globalGameState.playerP = newPlayerP;
  globalGameState.dPlayerP = newDPlayerP;

  V2 playerPInPixels = globalGameState.playerP * metersToPixels;

  V2 minPlayerRect = playerPInPixels;
  V2 maxPlayerRect =
      playerPInPixels + V2{playerWidthInPixels, playerHeightInPixels};

  if (globalGameState.playerIsInvulnerable &&
      ((timestamp - globalGameState.playerInvulnerableSince) > seconds(3))) {
    playerStopInvulnerable(&globalGameState);
  }
  // Render Player
  bool32 shouldRenderPlayer = true;
  if (globalGameState.playerIsInvulnerable) {
    u32 flashAnimationInterval = 125;
    u32 timeSinceInvulnerable =
        (u32)(timestamp - globalGameState.playerInvulnerableSince);
    u32 frameNumber = timeSinceInvulnerable / flashAnimationInterval;
    shouldRenderPlayer = (frameNumber % 2) == 0;
  }

  if (shouldRenderPlayer) {
    textureShaderDrawTexture(&textureShaderFrame, SPRITE_ATLAS_PLAYER,
                             minPlayerRect, maxPlayerRect);
  }

  // ENEMIES
  // Update Enemies
  f32 enemyBaseAcceleration = 20.0f;
  f32 enemyAIMinX = 0.3f * levelWidthInMeters;
  f32 enemyAIMaxX = levelWidthInMeters - enemyWidthInMeters * 1.2;

  for (int i = 0; i < arrayLength(globalGameState.enemies); ++i) {
    Enemy *currentEnemy = &globalGameState.enemies[i];

    if (currentEnemy->active) {
      // Compute Enemy Movement
      // Movement AI
      if ((timestamp - currentEnemy->aiModeLastChanged) > seconds(5)) {
        currentEnemy->aiMode = enemyPickAIMode();
      }
      switch (currentEnemy->aiMode) {
      case ENEMY_AI_MOVING_HORIZONTAL: {
        currentEnemy->intendedDirection.y = 0;
      } break;
      case ENEMY_AI_MOVING_UP: {
        currentEnemy->intendedDirection.y = 1;
      } break;
      case ENEMY_AI_MOVING_DOWN: {
        currentEnemy->intendedDirection.y = -1;
      } break;
      case ENEMY_AI_CHASING_PLAYER: {
        currentEnemy->intendedDirection.y = 0;
        if (globalGameState.playerP.y < currentEnemy->p.y) {
          currentEnemy->intendedDirection.y = -1;
        }
        if (globalGameState.playerP.y > currentEnemy->p.y) {
          currentEnemy->intendedDirection.y = 1;
        }
      } break;
      }

      if (currentEnemy->p.x < enemyAIMinX) {
        currentEnemy->intendedDirection.x = 1;
      }
      if (currentEnemy->p.x > enemyAIMaxX) {
        currentEnemy->intendedDirection.x = -1;
      }

      V2 normalizedDirection = currentEnemy->intendedDirection;
      if ((normalizedDirection.x != 0.0f) && (normalizedDirection.y != 0.0f)) {
        normalizedDirection *= 0.707186781197f;
      }

      V2 currentEnemyDdP = normalizedDirection * enemyBaseAcceleration;
      currentEnemyDdP += -2.5f * currentEnemy->dP;

      V2 newEnemyP = computeNewPosition(currentEnemy->p, currentEnemy->dP,
                                        currentEnemyDdP, dt);
      V2 newDEnemyP = computeNewVelocity(currentEnemy->dP, currentEnemyDdP, dt);

      // Collision with level boundaries
      f32 enemyMaxX = levelWidthInMeters - enemyWidthInMeters;
      f32 enemyMinX = 0;
      f32 enemyMaxY = levelHeightInMeters - enemyHeightInMeters;
      f32 enemyMinY = 0;

      CollisionResult boundariesCollision = getV2CollisionWithBoundaries(
          newEnemyP, newDEnemyP, V2{enemyMinX, enemyMinY},
          V2{enemyMaxX, enemyMaxY});

      newEnemyP = boundariesCollision.newPosition;
      newDEnemyP = boundariesCollision.newDPosition;

      currentEnemy->p = newEnemyP;
      currentEnemy->dP = newDEnemyP;

      // Bullet AI
      if ((timestamp - currentEnemy->bulletLastFired) > seconds(1)) {
        for (int i = 0; i < arrayLength(currentEnemy->bullets); i++) {
          Bullet *currentBullet = &currentEnemy->bullets[i];
          if (!currentBullet->firing) {
            currentBullet->firing = true;
            currentBullet->p =
                currentEnemy->p + V2{0.0f, enemyHeightInMeters * 0.375f -
                                               bulletWidthInMeters / 2};
            currentBullet->dP = {-30, 0};
            currentEnemy->bulletLastFired = timestamp;
            envPlayAudioShot();
            break;
          }
        }
      }
    }

    // Update bullets
    for (int i = 0; i < arrayLength(currentEnemy->bullets); ++i) {
      Bullet *currentBullet = &currentEnemy->bullets[i];

      if (currentBullet->firing) {
        // Compute Bullet Movement
        V2 ddBulletP = {0.0f, 0.0f};
        V2 newBulletP = computeNewPosition(currentBullet->p, currentBullet->dP,
                                           ddBulletP, dt);
        V2 newDBulletP = computeNewVelocity(currentBullet->dP, ddBulletP, dt);

        // Collision with Player
        if (!globalGameState.playerIsInvulnerable) {
          V2 playerTopRight = globalGameState.playerP +
                              V2{playerWidthInMeters, playerHeightInMeters};
          V2 bulletTopRight =
              newBulletP + V2{bulletWidthInMeters, bulletHeightInMeters};
          bool32 collidedWithPlayer =
              rectanglesAreColliding(globalGameState.playerP, playerTopRight,
                                     newBulletP, bulletTopRight);

          if (collidedWithPlayer) {
            currentBullet->firing = false;
            --globalGameState.playerLives;
            if (globalGameState.playerLives > 0) {
              playerStartInvulnerable(&globalGameState, timestamp);
            } else {
              globalGameState.gameOver = true;
              renderGameOver(&colorShaderFrame, &textureShaderFrame,
                             levelWidthInPixels, levelHeightInPixels);
              return;
            }
          }
        }

        if (newBulletP.x > 0) {
          currentBullet->p = newBulletP;
          currentBullet->dP = newDBulletP;
        } else {
          currentBullet->firing = false;
        }
      }
    }
  }

  // Update player bullets

  for (int i = 0; i < arrayLength(globalGameState.playerBullets); ++i) {
    Bullet *currentBullet = &globalGameState.playerBullets[i];

    if (currentBullet->firing) {
      // Compute Bullet Movement
      V2 ddBulletP = {0.0f, 0.0f};
      V2 newBulletP = computeNewPosition(currentBullet->p, currentBullet->dP,
                                         ddBulletP, dt);
      V2 newDBulletP = computeNewVelocity(currentBullet->dP, ddBulletP, dt);

      // Collision with enemies
      bool32 hasCollidedWithEnemy = false;
      // NOTE(fede): The leftmost enemy overlapping a bullet is the one that is
      // considered that collided
      Enemy *collidedEnemy;

      for (int i = 0; i < arrayLength(globalGameState.enemies); i++) {
        Enemy *enemy = &globalGameState.enemies[i];
        if (enemy->active) {
          V2 enemyTopRight =
              enemy->p + V2{enemyWidthInMeters, enemyHeightInMeters};
          V2 bulletTopRight =
              newBulletP + V2{bulletWidthInMeters, bulletHeightInMeters};
          bool32 collided = rectanglesAreColliding(enemy->p, enemyTopRight,
                                                   newBulletP, bulletTopRight);

          if (collided) {
            if (!hasCollidedWithEnemy) {
              hasCollidedWithEnemy = true;
              collidedEnemy = enemy;
            } else {
              if (collidedEnemy->p.x > enemy->p.x) {
                collidedEnemy = enemy;
              }
            }
          }
        }
      }

      if (hasCollidedWithEnemy) {
        currentBullet->firing = false;
        collidedEnemy->active = false;
        --globalGameState.enemiesCurrentCount;
      } else {
        // "Collision" with right wall
        if (newBulletP.x < levelWidthInMeters) {
          currentBullet->p = newBulletP;
          currentBullet->dP = newDBulletP;
        } else {
          currentBullet->firing = false;
        }
      }
    }
  }

  // Spawn Enemies
  bool32 enemyBufferFull = globalGameState.enemiesCurrentCount ==
                           arrayLength(globalGameState.enemies);

  // f64 randSeconds = 5.0f + envRandF32() * 10.0f;
  if (!enemyBufferFull && (globalGameState.enemyNextSpawn <= timestamp)) {
    // if(!enemyBufferFull && ((timestamp - globalGameState.enemyLastSpawned) >
    // seconds(5))) {
    //  Loop until we find a not active slot to spawn the enemy
    while (true) {
      Enemy *currentEnemy =
          &globalGameState.enemies[globalGameState.enemiesIndex];
      globalGameState.enemiesIndex++;
      if (globalGameState.enemiesIndex >=
          arrayLength(globalGameState.enemies)) {
        globalGameState.enemiesIndex = 0;
      }

      if (!currentEnemy->active) {
        currentEnemy->active = true;
        f32 spawnHeight =
            (levelHeightInMeters - enemyHeightInMeters) * envRandF32();
        currentEnemy->p = {levelWidthInMeters - enemyWidthInMeters,
                           spawnHeight};
        currentEnemy->dP = {-5.0f, 0};
        currentEnemy->intendedDirection = {-1, 0};
        currentEnemy->bulletLastFired = 0;
        currentEnemy->aiMode = ENEMY_AI_CHASING_PLAYER;
        currentEnemy->aiModeLastChanged = timestamp;
        break;
      }
    }

    f64 enemyNextSpawnSeconds = 2.0f + envRandF32() * 3.0f;
    globalGameState.enemyNextSpawn = timestamp + seconds(enemyNextSpawnSeconds);

    ++globalGameState.enemiesCurrentCount;
  }

  // Render Enemies
  for (int i = 0; i < arrayLength(globalGameState.enemies); ++i) {
    Enemy *currentEnemy = &globalGameState.enemies[i];

    // enemy
    if (currentEnemy->active) {
      // render;
      Color color = {0.0f, 1.0f, 1.0f, 1.0f};
      V2 bottomLeft = currentEnemy->p * metersToPixels;
      V2 relTopRight = {enemyWidthInPixels, enemyWidthInPixels};
      V2 enemyTopRight = bottomLeft + relTopRight;

      textureShaderDrawTexture(&textureShaderFrame, SPRITE_ATLAS_ENEMY_SHOOTER,
                               bottomLeft, enemyTopRight);
    }

    // bullets
    for (int i = 0; i < arrayLength(currentEnemy->bullets); ++i) {
      Bullet *currentBullet = &currentEnemy->bullets[i];

      if (currentBullet->firing) {
        if (currentBullet->p.x < levelWidthInMeters) {
          // render;
          Color color = {1.0f, 1.0f, 1.0f, 1};
          V2 min = currentBullet->p * metersToPixels;
          V2 relTopRight = {bulletWidthInPixels, bulletHeightInPixels};
          V2 bulletTopRight = min + relTopRight;

          colorShaderDrawRectangle(&colorShaderFrame, color, min,
                                   bulletTopRight);
        }
      }
    }
  }

  // Fire new player Bullets
  bool32 actionButtonToggledDown =
      globalGameControllerInputCurrent.action.isDown &&
      !globalGameControllerInputLastFrame.action.isDown;

  if (actionButtonToggledDown) {
    for (int i = 0; i < arrayLength(globalGameState.playerBullets); i++) {
      Bullet *currentBullet = &globalGameState.playerBullets[i];
      if (!currentBullet->firing) {
        currentBullet->firing = 1;
        currentBullet->p =
            globalGameState.playerP +
            V2{playerWidthInMeters,
               playerHeightInMeters * 0.375f - bulletWidthInMeters / 2};
        currentBullet->dP = {30, 0};
        envPlayAudioShot();
        break;
      }
    }
  }

  // Render player bullets
  for (int i = 0; i < arrayLength(globalGameState.playerBullets); ++i) {
    Bullet *currentBullet = &globalGameState.playerBullets[i];

    if (currentBullet->firing) {
      if (currentBullet->p.x < levelWidthInMeters) {
        // render;
        Color color = {1.0f, 1.0f, 1.0f, 1};
        V2 min = currentBullet->p * metersToPixels;
        V2 relTopRight = {bulletWidthInPixels, bulletHeightInPixels};
        V2 bulletTopRight = min + relTopRight;

        colorShaderDrawRectangle(&colorShaderFrame, color, min, bulletTopRight);
      }
    }
  }

  // Render player lives as hearts
  f32 heartSize = 32.0f; // Size of heart sprite in pixels
  f32 heartPadding = 8.0f; // Padding between hearts
  f32 heartStartX = 16.0f; // Starting X position from left edge
  f32 heartY = levelHeightInPixels - heartSize - 16.0f; // Y position from top
  
  for (u32 i = 0; i < globalGameState.playerLives; ++i) {
    V2 heartBottomLeft = {heartStartX + i * (heartSize + heartPadding), heartY};
    V2 heartTopRight = heartBottomLeft + V2{heartSize, heartSize};
    
    textureShaderDrawTexture(&textureShaderFrame, SPRITE_ATLAS_HEART,
                             heartBottomLeft, heartTopRight);
  }

  // Update globals
  globalColorShaderFrameTrianglesCount = colorShaderFrame.trianglesCount;
  globalTextureShaderFrameTrianglesCount = textureShaderFrame.trianglesCount;

  globalLastTimestamp = timestamp;
  globalGameControllerInputLastFrame = globalGameControllerInputCurrent;
}

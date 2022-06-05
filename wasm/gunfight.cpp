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
void bufferPushf32(Buffer *buffer, f32 value) {
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
    globalGameState.playerP = {1.0f, 5.0f};
    globalGameState.dPlayerP = {};

    for(int i = 0; i < arrayLength(globalGameState.playerBullets); ++i) {
      globalGameState.playerBullets[i] = {
        {50, 50},
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

  f32 baseAcceleration = 20.0f;

  ddPlayerP *= baseAcceleration;

  // Add "Friction"
  ddPlayerP += -2.5f*globalGameState.dPlayerP;

  f32 dt = (f32)((timestamp - globalLastTimestamp) / 1000.0f); // in seconds

  V2 newPlayerP = globalGameState.playerP + 0.5f*ddPlayerP*square(dt) + globalGameState.dPlayerP * dt;
  V2 newDPlayerP = ddPlayerP * dt + globalGameState.dPlayerP;

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

  V2 positionInPixels = globalGameState.playerP * metersToPixels;

  f32 minX = positionInPixels.x;
  f32 minY = positionInPixels.y;
  f32 maxX = positionInPixels.x + playerWidthInPixels;
  f32 maxY = positionInPixels.y + playerHeightInPixels;

  textureShaderDrawPlayer(&textureShaderFrame, minX, minY, maxX, maxY);

  // Bullets
  bool32 actionButtonToggledDown = 
    globalGameControllerInputCurrent.action.isDown && !globalGameControllerInputLastFrame.action.isDown;

  if(actionButtonToggledDown) {
    for(int i = 0; i < arrayLength(globalGameState.playerBullets); i++) {
      Bullet *currentBullet = &globalGameState.playerBullets[i];
      if(!currentBullet->firing) {
        currentBullet->firing = 1;
        currentBullet->p = globalGameState.playerP;
        currentBullet->dP = {100, 0};
        break;
      }
    }
  }

  for(int i = 0; i < arrayLength(globalGameState.playerBullets); ++i) {
    Bullet currentBullet = globalGameState.playerBullets[i];

    if(currentBullet.firing) {
      Color color = {1.0f, 1.0f, 1.0f, 1};
      // TODO: pasar esto a vectores
      f32 bulletWidthInMeters = 0.2f; 
      f32 bulletWidthInPixels = bulletWidthInMeters * metersToPixels;
      V2 min = currentBullet.p * metersToPixels;
      V2 relTopRight = {bulletWidthInPixels, bulletWidthInPixels};
      V2 bulletTopRight = min + relTopRight;

      colorShaderDrawRectangle(&colorShaderFrame, color, min, bulletTopRight);
    }
  }

  // Update globals
  globalColorShaderFrameTrianglesCount = colorShaderFrame.trianglesCount; 
  globalTextureShaderFrameTrianglesCount = textureShaderFrame.trianglesCount; 

  globalLastTimestamp = timestamp;
  globalGameControllerInputLastFrame = globalGameControllerInputCurrent;
}

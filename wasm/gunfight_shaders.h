#include "gunfight_sprite_atlas.h" 

// Data about what the color shader should draw on the next frame
struct ColorShaderFrame {
  u32 trianglesCount;
  Buffer aPositionBuffer;
  Buffer uColorsBuffer;
};

// Data for the texture shader to draw on the current frame
struct TextureShaderFrame {
  u32 trianglesCount;
  Buffer aPositionBuffer;
  Buffer aTexCoordBuffer;
}; 

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

// DRAW
internal void colorShaderDrawRectangle(
        ColorShaderFrame *colorShaderFrame,
        Color color,
        V2 min,
        V2 max
) {
  f32 vertices[12] = {
    min.x, min.y,
    min.x, max.y,
    max.x, max.y,
    min.x, min.y,
    max.x, min.y,
    max.x, max.y,
  };

  Buffer *aPositionBuffer = &colorShaderFrame->aPositionBuffer;
  Buffer *uColorsBuffer = &colorShaderFrame->uColorsBuffer;

  for(int i = 0; i < arrayLength(vertices); i++) {
    bufferPushF32(aPositionBuffer, vertices[i]);
  }

  // every three vertices pairs (triangle) we need to specify color and increment triangles
  // count
  for(int i = 0; i < arrayLength(vertices)/6; i++) {
    bufferPushF32(uColorsBuffer, color.r);
    bufferPushF32(uColorsBuffer, color.g);
    bufferPushF32(uColorsBuffer, color.b);
    bufferPushF32(uColorsBuffer, color.a);

    colorShaderFrame->trianglesCount++;
  }
}

internal void setATexCoordValsFromTextureIndex(int textureIndex, Buffer *buffer) {
  AtlasTextureMetadata textureMetadata =
    globalSpriteAtlasMetadata.texturesMetadata[textureIndex];

  for(int i = 0; i<12; ++i) {
    bufferPushF32(buffer, textureMetadata.textureCoordinates[i]);
  }
}

internal void textureShaderDrawTexture(
    TextureShaderFrame *textureShaderFrame,
    int textureIndex,
    V2 min,
    V2 max
) {
  f32 aPositionVals[12] = {
    min.x, min.y,
    max.x, min.y,
    min.x, max.y,
    min.x, max.y,
    max.x, min.y,
    max.x, max.y,
  };

  Buffer *aPositionBuffer = &textureShaderFrame->aPositionBuffer;

  for(int i = 0; i < arrayLength(aPositionVals); i++) {
    bufferPushF32(aPositionBuffer, aPositionVals[i]);
  }

  setATexCoordValsFromTextureIndex(
      textureIndex,
      &textureShaderFrame->aTexCoordBuffer
  );

  textureShaderFrame->trianglesCount += 2;
}

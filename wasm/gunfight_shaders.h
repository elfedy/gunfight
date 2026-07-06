#include "gunfight_charset.h"
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

// Data for the texture shader to draw on the current frame
struct FontShaderFrame {
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

FontShaderFrame fontShaderFrameInit() {
  FontShaderFrame ret = {};
  ret.trianglesCount = 0;

  Buffer aPositionBuffer = {};
  aPositionBuffer.current = getBufferBase(INDEX_FONT_SHADER_A_POSITION);
  aPositionBuffer.offset = 0;

  Buffer aTexCoordBuffer = {};

  aTexCoordBuffer.current = getBufferBase(INDEX_FONT_SHADER_A_TEX_COORD);
  aTexCoordBuffer.offset = 0;

  ret.aPositionBuffer = aPositionBuffer;
  ret.aTexCoordBuffer = aTexCoordBuffer;

  return ret;
}

// DRAW
internal void pushRectangleVertices(V2 min, V2 max, Buffer *buffer) {
  bufferPushF32(buffer, min.x);
  bufferPushF32(buffer, min.y);

  bufferPushF32(buffer, max.x);
  bufferPushF32(buffer, min.y);

  bufferPushF32(buffer, min.x);
  bufferPushF32(buffer, max.y);

  bufferPushF32(buffer, min.x);
  bufferPushF32(buffer, max.y);

  bufferPushF32(buffer, max.x);
  bufferPushF32(buffer, min.y);

  bufferPushF32(buffer, max.x);
  bufferPushF32(buffer, max.y);
}

internal void colorShaderDrawRectangle(ColorShaderFrame *colorShaderFrame,
                                       Color color, V2 min, V2 max) {
  Buffer *aPositionBuffer = &colorShaderFrame->aPositionBuffer;
  Buffer *uColorsBuffer = &colorShaderFrame->uColorsBuffer;

  pushRectangleVertices(min, max, aPositionBuffer);

  // every three vertices pairs (triangle) we need to specify color and
  // increment triangles count
  for (int i = 0; i < 2; i++) {
    bufferPushF32(uColorsBuffer, color.r);
    bufferPushF32(uColorsBuffer, color.g);
    bufferPushF32(uColorsBuffer, color.b);
    bufferPushF32(uColorsBuffer, color.a);

    colorShaderFrame->trianglesCount++;
  }
}

internal void setATexCoordValsFromTextureIndex(int textureIndex,
                                               Buffer *buffer) {
  AtlasTextureMetadata textureMetadata =
      globalSpriteAtlasMetadata.texturesMetadata[textureIndex];

  for (int i = 0; i < 12; ++i) {
    bufferPushF32(buffer, textureMetadata.textureCoordinates[i]);
  }
}

internal void textureShaderDrawTexture(TextureShaderFrame *textureShaderFrame,
                                       int textureIndex, V2 min, V2 max) {
  Buffer *aPositionBuffer = &textureShaderFrame->aPositionBuffer;

  pushRectangleVertices(min, max, aPositionBuffer);

  setATexCoordValsFromTextureIndex(textureIndex,
                                   &textureShaderFrame->aTexCoordBuffer);

  textureShaderFrame->trianglesCount += 2;
}

internal void setATexCoordValsFromCharsetCode(u32 code, Buffer *buffer) {
  assert(code < arrayLength(globalCharsetMetadata.glyphs));

  CharsetGlyphMetadata glyphMetadata = globalCharsetMetadata.glyphs[code];

  f32 minU = (f32)glyphMetadata.x / (f32)globalCharsetMetadata.totalWidth;
  f32 minV = (f32)glyphMetadata.y / (f32)globalCharsetMetadata.totalHeight;
  f32 maxU = (f32)(glyphMetadata.x + globalCharsetMetadata.glyphWidth) /
             (f32)globalCharsetMetadata.totalWidth;
  f32 maxV = (f32)(glyphMetadata.y + globalCharsetMetadata.glyphHeight) /
             (f32)globalCharsetMetadata.totalHeight;

  bufferPushF32(buffer, minU);
  bufferPushF32(buffer, minV);

  bufferPushF32(buffer, maxU);
  bufferPushF32(buffer, minV);

  bufferPushF32(buffer, minU);
  bufferPushF32(buffer, maxV);

  bufferPushF32(buffer, minU);
  bufferPushF32(buffer, maxV);

  bufferPushF32(buffer, maxU);
  bufferPushF32(buffer, minV);

  bufferPushF32(buffer, maxU);
  bufferPushF32(buffer, maxV);

}

internal void fontShaderDrawCharset(FontShaderFrame *fontShaderFrame,
                                    const char *text, V2 min,
                                    f32 sizeMultiplier) {
  f32 cursorX = min.x;
  f32 cursorY = min.y;
  f32 lineStartX = min.x;
  f32 glyphWidth = (f32)globalCharsetMetadata.glyphWidth * sizeMultiplier;
  f32 glyphHeight = (f32)globalCharsetMetadata.glyphHeight * sizeMultiplier;
  f32 defaultAdvance = glyphWidth;
  f32 lineHeight = glyphHeight;

  for (const char *current = text; *current != '\0'; ++current) {
    u32 code = (u8)*current;

    if (code == '\n') {
      cursorX = lineStartX;
      cursorY += lineHeight;
      continue;
    }

    if (code == ' ') {
      cursorX += defaultAdvance;
      continue;
    }

    setATexCoordValsFromCharsetCode(code, &fontShaderFrame->aTexCoordBuffer);

    V2 glyphMin = {cursorX, cursorY};
    V2 glyphMax = {cursorX + glyphWidth, cursorY + glyphHeight};

    Buffer *aPositionBuffer = &fontShaderFrame->aPositionBuffer;
    pushRectangleVertices(glyphMin, glyphMax, aPositionBuffer);

    fontShaderFrame->trianglesCount += 2;
    cursorX += glyphWidth;
  }
}

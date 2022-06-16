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

// TODO(fede): For now textures are hardcoded and there's no identifier to
// determien which texture is which. Maybe it is worth it to build a system
// of a dynamic set of textures where all data is read from a header and
// texture metadata can be fetched by key
struct AtlasTextureMetadata {
  f32 x;
  f32 y;
  f32 width;
  f32 height;
};

struct SpriteAtlasMetadata {
  u32 totalWidth;
  u32 totalHeight;
  AtlasTextureMetadata texturesMetadata[2];
};

global_variable SpriteAtlasMetadata globalSpriteAtlasMetadata = {
  32,
  16,
  {
    // player
    {
      0,
      0,
      16,
      16
    },
    // enemy shooter
    {
      16,
      0,
      16,
      16
    }
  }
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
  f32 minX = textureMetadata.x / globalSpriteAtlasMetadata.totalWidth;
  f32 minY = textureMetadata.y / globalSpriteAtlasMetadata.totalHeight;
  f32 maxX = (textureMetadata.x + textureMetadata.width) / globalSpriteAtlasMetadata.totalWidth;
  f32 maxY = (textureMetadata.y + textureMetadata.height) / globalSpriteAtlasMetadata.totalHeight;

  f32 vals[12] = {
    minX, minY,
    maxX, minY,
    minX, maxY,
    minX, maxY,
    maxX, minY,
    maxX, maxY,
  }; 

  for(int i = 0; i<12; ++i) {
    bufferPushF32(buffer, vals[i]);
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

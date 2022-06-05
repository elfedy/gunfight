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
    bufferPushf32(aPositionBuffer, vertices[i]);
  }

  // every three vertices pairs (triangle) we need to specify color and increment triangles
  // count
  for(int i = 0; i < arrayLength(vertices)/6; i++) {
    bufferPushf32(uColorsBuffer, color.r);
    bufferPushf32(uColorsBuffer, color.g);
    bufferPushf32(uColorsBuffer, color.b);
    bufferPushf32(uColorsBuffer, color.a);

    colorShaderFrame->trianglesCount++;
  }
}

internal void textureShaderDrawPlayer(
    TextureShaderFrame *textureShaderFrame,
    f32 minX,
    f32 minY,
    f32 maxX,
    f32 maxY
) {
  f32 aPositionVals[12] = {
    minX, minY,
    maxX, minY,
    minX, maxY,
    minX, maxY,
    maxX, minY,
    maxX, maxY,
  };

  f32 aTexCoordVals[12] = {
      0.0f,  0.0f,
      1.0f,  0.0f,
      0.0f,  1.0f,
      0.0f,  1.0f,
      1.0f,  0.0f,
      1.0f,  1.0f
  };

  Buffer *aPositionBuffer = &textureShaderFrame->aPositionBuffer;
  Buffer *aTexCoordBuffer = &textureShaderFrame->aTexCoordBuffer;

  for(int i = 0; i < arrayLength(aPositionVals); i++) {
    bufferPushf32(aPositionBuffer, aPositionVals[i]);
  }

  for(int i = 0; i < arrayLength(aTexCoordVals); i++) {
    bufferPushf32(aTexCoordBuffer, aTexCoordVals[i]);
  }

  textureShaderFrame->trianglesCount += 2;
}
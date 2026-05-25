const fs = require('fs');

const CHARSET_PATH = './assets/charset.json';
const OUTPUT_PATH = './wasm/gunfight_charset.h';
const ATLAS_TOTAL_WIDTH = 512;
const ATLAS_TOTAL_HEIGHT = 192;
const ASCII_GLYPH_COUNT = 128;

const charset = JSON.parse(fs.readFileSync(CHARSET_PATH, 'utf8'));

const glyphs = Array.from({ length: ASCII_GLYPH_COUNT }, (_, code) => ({
  code,
  x: 0,
  y: 0,
  width: 0,
  height: 0,
  textureCoordinates: Array(12).fill(0),
}));

const getTextureCoordinates = (glyphMetadata) => {
  const minX = glyphMetadata.x / ATLAS_TOTAL_WIDTH;
  const maxX = (glyphMetadata.x + glyphMetadata.w) / ATLAS_TOTAL_WIDTH;
  const minY = glyphMetadata.y / ATLAS_TOTAL_HEIGHT;
  const maxY = (glyphMetadata.y + glyphMetadata.h) / ATLAS_TOTAL_HEIGHT;

  return [
    minX, minY,
    minX, maxY,
    maxX, minY,
    minX, maxY,
    maxX, maxY,
    maxX, minY,
  ];
};

Object.values(charset).forEach((glyphMetadata) => {
  const code = glyphMetadata.code;
  if (code < 0 || code >= ASCII_GLYPH_COUNT) {
    return;
  }

  glyphs[code] = {
    code,
    x: glyphMetadata.x,
    y: glyphMetadata.y,
    width: glyphMetadata.w,
    height: glyphMetadata.h,
    textureCoordinates: getTextureCoordinates(glyphMetadata),
  };
});

const formatTextureCoordinates = (textureCoordinates) => `{
      ${textureCoordinates[0]}, ${textureCoordinates[1]},
      ${textureCoordinates[2]}, ${textureCoordinates[3]},
      ${textureCoordinates[4]}, ${textureCoordinates[5]},
      ${textureCoordinates[6]}, ${textureCoordinates[7]},
      ${textureCoordinates[8]}, ${textureCoordinates[9]},
      ${textureCoordinates[10]}, ${textureCoordinates[11]}
    }`;

const formatGlyph = (glyph) => `{
    ${glyph.code},
    ${glyph.x},
    ${glyph.y},
    ${glyph.width},
    ${glyph.height},
    ${formatTextureCoordinates(glyph.textureCoordinates)}
  }`;

const charsetFileContent = `#if !defined(GUNFIGHT_CHARSET)
struct CharsetGlyphMetadata {
  u32 code;
  u32 x;
  u32 y;
  u32 width;
  u32 height;

  f32 textureCoordinates[12];
};

struct CharsetMetadata {
  u32 totalWidth;
  u32 totalHeight;
  CharsetGlyphMetadata glyphs[${ASCII_GLYPH_COUNT}];
};

global_variable CharsetMetadata globalCharsetMetadata = {
  ${ATLAS_TOTAL_WIDTH},
  ${ATLAS_TOTAL_HEIGHT},
  {${glyphs.map((glyph) => formatGlyph(glyph)).join(',\n  ')}}
};

#define GUNFIGHT_CHARSET
#endif
`;

fs.writeFileSync(OUTPUT_PATH, charsetFileContent, { encoding: 'utf8', flag: 'w' });

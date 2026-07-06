const fs = require('fs');

const CHARSET_PATH = './assets/charset.json';
const OUTPUT_PATH = './wasm/gunfight_charset.h';
const ATLAS_TOTAL_WIDTH = 512;
const ATLAS_TOTAL_HEIGHT = 192;
const GLYPH_WIDTH = 32;
const GLYPH_HEIGHT = 32;
const ASCII_GLYPH_COUNT = 128;
const INVALID_GLYPH_POSITION = 4294967295;

const charset = JSON.parse(fs.readFileSync(CHARSET_PATH, 'utf8'));

const glyphs = Array.from({ length: ASCII_GLYPH_COUNT }, (_, code) => ({
  code,
  x: INVALID_GLYPH_POSITION,
  y: INVALID_GLYPH_POSITION,
}));

Object.values(charset).forEach((glyphMetadata) => {
  const code = glyphMetadata.code;
  if (code < 0 || code >= ASCII_GLYPH_COUNT) {
    return;
  }

  if (glyphMetadata.w !== GLYPH_WIDTH || glyphMetadata.h !== GLYPH_HEIGHT) {
    throw new Error(`Unexpected glyph size for code ${code}: ${glyphMetadata.w}x${glyphMetadata.h}`);
  }

  glyphs[code] = {
    code,
    x: glyphMetadata.x,
    y: glyphMetadata.y,
  };
});

const formatGlyph = (glyph) => `{
    ${glyph.code},
    ${glyph.x},
    ${glyph.y}
  }`;

const charsetFileContent = `#if !defined(GUNFIGHT_CHARSET)
struct CharsetGlyphMetadata {
  u32 code;
  u32 x;
  u32 y;
};

struct CharsetMetadata {
  u32 totalWidth;
  u32 totalHeight;
  u32 glyphWidth;
  u32 glyphHeight;
  CharsetGlyphMetadata glyphs[${ASCII_GLYPH_COUNT}];
};

global_variable CharsetMetadata globalCharsetMetadata = {
  ${ATLAS_TOTAL_WIDTH},
  ${ATLAS_TOTAL_HEIGHT},
  ${GLYPH_WIDTH},
  ${GLYPH_HEIGHT},
  {${glyphs.map((glyph) => formatGlyph(glyph)).join(',\n  ')}}
};

#define GUNFIGHT_CHARSET
#endif
`;

fs.writeFileSync(OUTPUT_PATH, charsetFileContent, { encoding: 'utf8', flag: 'w' });

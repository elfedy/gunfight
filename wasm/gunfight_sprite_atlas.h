#if !defined(GUNFIGHT_SPRITE_ATLAS)
enum TextureIndex {
  SPRITE_ATLAS_PLAYER,
  SPRITE_ATLAS_ENEMY_SHOOTER,
  SPRITE_ATLAS_HEART
};

struct AtlasTextureMetadata {
  f32 xFrac;
  f32 yFrac;
  f32 width;
  f32 height;

  f32 textureCoordinates[12];
};

struct SpriteAtlasMetadata {
  u32 totalWidth;
  u32 totalHeight;
  AtlasTextureMetadata texturesMetadata[3];
};

global_variable SpriteAtlasMetadata globalSpriteAtlasMetadata = {
  16,
  48,
  {{
    0,
    0.6666666666666666,
    16,
    16,
    {
      0, 0.6666666666666666,
      1, 0.6666666666666666,
      0, 1,
      0, 1,
      1, 0.6666666666666666,
      1, 1
    }
  },
  {
    0,
    0.3333333333333333,
    16,
    16,
    {
      0, 0.3333333333333333,
      1, 0.3333333333333333,
      0, 0.6666666666666666,
      0, 0.6666666666666666,
      1, 0.3333333333333333,
      1, 0.6666666666666666
    }
  },
  {
    0,
    0,
    16,
    16,
    {
      0, 0,
      1, 0,
      0, 0.3333333333333333,
      0, 0.3333333333333333,
      1, 0,
      1, 0.3333333333333333
    }
  }}
};

#define GUNFIGHT_SPRITE_ATLAS
#endif

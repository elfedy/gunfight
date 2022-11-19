const fs = require('fs');
const { exec } = require('child_process');
const path = require('path');

const IMAGE_PATH = './dev/sprite_atlas_assets';

const imageMetadata =  [
  {
   filename: 'player.png',
   id: 'PLAYER',
  },
  {
   filename: 'enemy_shooter.png',
   id: 'ENEMY_SHOOTER',
  },
  {
    filename: 'heart.png',
    id: 'HEART',
  },
]

const filepath = (metadata) => path.join(IMAGE_PATH, metadata.filename);

imageMetadata.forEach(metadata => {
  const file_fd = fs.openSync(filepath(metadata), 'r');
  const { size } = fs.fstatSync(file_fd);
  const buffer = Buffer.alloc(size);
  fs.readSync(file_fd, buffer, 0, size, 0);

  if(buffer.toString('ascii', 12, 16) === 'CgBI') {
    metadata.width = buffer.readUInt32BE(32);
    metadata.height = buffer.readUInt32BE(36);
  } else {
    metadata.width = buffer.readUInt32BE(16);
    metadata.height = buffer.readUInt32BE(20);
  }
});

let totalWidth = 0;
let totalHeight = 0;

// NOTE(fede): image magick builds montage first image on top, but WebGL places the y axis at the bottom so we need
// to compute y starting from the bottom
const imageMetadataReversed = imageMetadata.slice().reverse();
imageMetadataReversed.forEach((metadata, index) => {
  totalWidth = Math.max(totalWidth, metadata.width);
  totalHeight += metadata.height;

  metadata.x = 0;
  if(index === 0) {
    metadata.y = 0;
  } else {
    const previousMetadata = imageMetadataReversed[index - 1];
    metadata.y = previousMetadata.y + previousMetadata.height;
  }
});

// TODO: Armar el archivo en c
const command = `montage ${imageMetadata.map(metadata => filepath(metadata)).join(' ')} -tile 1x -geometry +0+0 -background none ./assets/sprite_atlas.png`;
exec(command, (error, stdout, stderr) => {
    console.log(stdout);
    if(error) {
      console.log(`error: ${error.message}`);
    }
    if(stderr) {
      console.log(`stderr: ${stderr}`);
    }
})

let textureIndices = imageMetadata.map(id => 'SPRITE_ATLAS_' + id.id).join(',\n  ');

const getSpriteAtlasMetadata = (textureMetadata, atlasTotalWidth, atlasTotalHeight) => {
  const minX  = (textureMetadata.x / atlasTotalWidth);
  const minY  = (textureMetadata.y / atlasTotalHeight);
  const maxX = (textureMetadata.x + textureMetadata.width) / atlasTotalWidth;
  const maxY = (textureMetadata.y + textureMetadata.height) / atlasTotalHeight;

  const coordinates = `{
      ${minX}, ${minY},
      ${maxX}, ${minY},
      ${minX}, ${maxY},
      ${minX}, ${maxY},
      ${maxX}, ${minY},
      ${maxX}, ${maxY}
    }`;

  return `{
    ${minX},
    ${minY},
    ${textureMetadata.width},
    ${textureMetadata.width},
    ${coordinates}
  }`;
}


const spriteAtlasFileContent = `#if !defined(GUNFIGHT_SPRITE_ATLAS)
enum TextureIndex {
  ${textureIndices}
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
  AtlasTextureMetadata texturesMetadata[${imageMetadata.length}];
};

global_variable SpriteAtlasMetadata globalSpriteAtlasMetadata = {
  ${totalWidth},
  ${totalHeight},
  {${imageMetadata.map(im => getSpriteAtlasMetadata(im, totalWidth, totalHeight)).join(",\n  ")}}
};

#define GUNFIGHT_SPRITE_ATLAS
#endif
`

fs.writeFileSync('./wasm/gunfight_sprite_atlas.h', spriteAtlasFileContent, {encoding:'utf8',flag:'w'})

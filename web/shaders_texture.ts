// TODO: Renderear al tipo con el shader de los comments
function textureShaderSetup(gl) {
  let vertexShaderSource = `
      attribute vec2 aPosition;
      attribute vec2 aTexCoord;
      uniform mat3 uMatrix;
      varying vec2 vTexCoord;
      
      void main() {
        gl_Position = vec4((uMatrix * vec3(aPosition, 1)).xy, 0, 1);
        vTexCoord = aTexCoord;
      }
    `;
  
  let fragmentShaderSource = `
      precision mediump float;

      // This is never set in the code as it defaults to using 
      // texture unit 0
      uniform sampler2D uImage;
      varying vec2 vTexCoord;

      void main() {
        gl_FragColor = texture2D(uImage, vTexCoord);
      }
    `;

  let shaderProgram = initShaderProgram(gl, vertexShaderSource, fragmentShaderSource);

  let shaderInfo = {
    program: shaderProgram,
    buffers: {
      aPosition: gl.createBuffer(),
      aTexCoord: gl.createBuffer(),
    },
    locations: {
      aPosition: gl.getAttribLocation(shaderProgram, "aPosition"),
      aTexCoord: gl.getAttribLocation(shaderProgram, "aTexCoord"),
      uMatrix: gl.getUniformLocation(shaderProgram, "uMatrix"),
      uImage: gl.getUniformLocation(shaderProgram, "uImage"),
    },
    textures: {
      sprite: gl.createTexture(),
    }
  };

  // Enable indices at vertex attributes to be interpreted as such
  gl.enableVertexAttribArray(shaderInfo.locations.aPosition);
  gl.enableVertexAttribArray(shaderInfo.locations.aTexCoord);

  return shaderInfo;
}

function textureShaderSetTexture(gl, glTargetTexture: string, shaderInfo, image) {
  // Make shader texture the active texture
  // Make the target texture the active gl texture
  // TODO: Ver como esto se relaciona con los targets
  gl.activeTexture(gl[glTargetTexture]);
  // Bind the sprite texture to TEXTURE_2D binding point
  gl.bindTexture(gl.TEXTURE_2D, shaderInfo.textures.sprite);

  // Set texture parameters
  gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE)
  gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE)
  gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST)
  gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST)

  // Upload sprite image to the GPU's texture object
  // TODO: ver esto como se relaciona con las active textures
  gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, gl.RGBA, gl.UNSIGNED_BYTE, image)

  // Add some magic stuff to make alpha in images blend with the rest.
  gl.pixelStorei(gl.UNPACK_PREMULTIPLY_ALPHA_WEBGL, true);
  gl.enable(gl.BLEND);
  gl.blendFunc(gl.SRC_ALPHA, gl.ONE_MINUS_SRC_ALPHA);
}

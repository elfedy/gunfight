function colorShaderSetup(gl) {
  let vertexShaderSource = `
      attribute vec2 aPosition;
      uniform mat3 uMatrix;
      
      void main() {
        gl_Position = vec4((uMatrix * vec3(aPosition, 1)).xy, 0, 1);
      }
    `;

  let fragmentShaderSource = `
      precision mediump float;

      uniform vec4 uColor;

      void main() {
        gl_FragColor = uColor;
      }
    `;

  let colorShaderProgram = initShaderProgram(gl, vertexShaderSource, fragmentShaderSource);

  let shaderInfo = {
    program: colorShaderProgram,
    buffers: {
      aPosition: gl.createBuffer(),
    },
    locations: {
      aPosition: gl.getAttribLocation(colorShaderProgram, "aPosition"),
      uColor: gl.getUniformLocation(colorShaderProgram, "uColor"),
      uMatrix: gl.getUniformLocation(colorShaderProgram, "uMatrix"),
    }
  };

  // Enable aPosition index to be interpreted as a vertex
  // attribute
  gl.enableVertexAttribArray(shaderInfo.locations.aPosition);

  return shaderInfo;
}

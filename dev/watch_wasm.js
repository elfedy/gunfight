const	chokidar = require('chokidar');
const { exec } = require('child_process');
const path = require('path');

let dir = './wasm';
let command = './build_wasm.sh';

console.log(`Started watching ${dir}`);

chokidar.watch(dir).on('change', filepath => {
    if(path.extname(filepath) === '.c') {
      console.log(`Detected change in ${path}.`);
      try {
        exec(command, (error, stdout, stderr) => {
          console.log(stdout);
          if(error) {
            console.log(`error: ${error.message}`);
          }
          if(stderr) {
            console.log(`stderr: ${stderr}`);
          }
        })
      } catch(e) {
        console.log(`FAILED: ${e}`);
      }
    }
  }
);

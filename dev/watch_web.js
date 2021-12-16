const	chokidar = require('chokidar');
const { exec } = require('child_process');

let dir = './web';
let command = './build_web.sh';

console.log(`Started watching ${dir}`);

chokidar.watch(dir).on('change', (path) => {
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
);

const	fsevents = require('fsevents');
const { exec } = require('child_process');

console.log("Started watching ./wasm");
let locked = false;
const stop = fsevents.watch('./wasm', (path, flags, id) => {
  const info = fsevents.getInfo(path, flags, id);
  if(info.event === 'moved') {
    console.log(`Detected change in ${info.path}.`);
    if(!locked) {
      locked = true;
      exec('./build_wasm.sh', (error, stdout, stderr) => {
        console.log(stdout);
        if(error) {
          console.log(`error: ${error.message}`);
        }
        if(stderr) {
          console.log(`stderr: ${stderr}`);
        }
        locked = false;
      })
    } else {
      console.log("Build is locked");
    }

  }
})

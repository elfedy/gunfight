const	fsevents = require('fsevents');
const { exec } = require('child_process');

console.log("Started watching ./wasm");
const stop = fsevents.watch('./wasm', (path, flags, id) => {
  const info = fsevents.getInfo(path, flags, id);
  if(info.event === 'moved') {
    console.log(`Detected change in ${info.path}.`);
    try {
      exec('./build_wasm.sh', (error, stdout, stderr) => {
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
})

const	fsevents = require('fsevents');
const { exec } = require('child_process');

console.log("Started watching /src directory...");


['./web', './wasm'].forEach(dir => {
  const stop = fsevents.watch(dir, (path, flags, id) => {
    const info = fsevents.getInfo(path, flags, id);
    if(info.event === 'moved') {
      console.log(`Detected change in ${info.path}.`);
      exec('./build.sh', (error, stdout, stderr) => {
        console.log(stdout);
        if(error) {
          console.log(`error: ${error.message}`);
        }
        if(stderr) {
          console.log(`stderr: ${stderr}`);
        }
      })
    }
  })
});


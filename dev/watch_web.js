const	fsevents = require('fsevents');
const { exec } = require('child_process');

let dir = './web';
let command = './build_web.sh';

console.log(`Started watching ./dir`);

const stop = fsevents.watch('./web', (path, flags, id) => {
  const info = fsevents.getInfo(path, flags, id);
  if(info.event === 'moved') {
    console.log(`Detected change in ${info.path}.`);
    exec(command, (error, stdout, stderr) => {
      console.log(stdout);
      if(error) {
        console.log(`error: ${error.message}`);
      }
      if(stderr) {
        console.log(`stderr: ${stderr}`);
      }
    })
  }
});

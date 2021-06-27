const	fsevents = require('fsevents');
const { exec } = require('child_process');

console.log("Started watching /src directory...");

const stop = fsevents.watch('./src', (path, flags, id) => {
	const info = fsevents.getInfo(path, flags, id);
	if(info.event === 'moved') {
		console.log(`Detected change in ${info.path}.`);
		exec('./build.sh dev', (error, stdout, stderr) => {
			if(error) {
				console.log(`error: ${error.message}`);
			}
			if(stderr) {
				console.log(`srderr: ${stderr}`);
			}

			console.log(stdout);
		})
	}
})

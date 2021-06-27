const http = require('http');
const fs = require('fs');
const path = require('path');

process.on('uncaughtException', err => console.error('uncaught exception', err));
process.on('unhandledRejection', err => console.error('unhandled rejection', err));

const publicFolder = process.argv.length > 2 ? process.argv[2] : '.';
const port = process.argv.length > 3 ? process.argv[3] : 8080;

const mediaTypes = {
  png: 'image/png',
  html: 'text/html',
  wasm: 'application/wasm',
};

const server = http.createServer(function(request, response) {
  console.log(request.method + ' ' + request.url);

  let targetUrl = request.url;
  if(targetUrl === '/') {
    targetUrl = '/index.html'
  }

  const filepath = path.join(publicFolder, targetUrl);

  fs.readFile(filepath, function(err, data) {
    if(err) {
      response.statusCode = 404;
      return response.end('Not found');
    }

    let mediaType = 'text/html';
    const ext = path.extname(filepath)
    if(ext.length > 0 && mediaTypes.hasOwnProperty(ext.slice(1))) {
      mediaType = mediaTypes[ext.slice(1)];
    }

    response.setHeader('Content-Type', mediaType);
    response.end(data);
  })
})

server.on('clientError', function onClientError(err, socket) {
  console.log('clientError', err);
  socket.end('HTTP/1.1 400 Bad Request\r\n\r\n');
})

server.listen(port, '127.0.0.1', function() {
  console.log('Started dev server on port: ' + port);
})

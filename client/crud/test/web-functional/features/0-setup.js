const _ = require('lodash');
const http = require('http');
const fs = require('fs');
const path = require('path');


const DIST_DIR = path.resolve(__dirname, '../../../web/dist');

const server = http.createServer(function (request, response) {

    const filename = path.resolve(`${DIST_DIR}/${request.url}`);
    fs.existsSync(filename) && fs.lstatSync(filename).isFile() ? sendFile(filename) : sendFile(`${DIST_DIR}/index.html`);

    function sendFile(filename) {
        response.writeHead(200);
        response.end(fs.readFileSync(filename));
    }
});

server.listen(8200);
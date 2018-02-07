const http = require('http');
const fs = require('fs');
const path = require('path');


const DIST_DIR = __dirname + '/test-app/dist';

if(!fs.existsSync(`${DIST_DIR}/bundle.js`)) {
    throw 'Please `cd test/test-app` and `yarn build`.';
}


let server;

export const start = function(port=8200) {
    server = http.createServer(function (request, response) {

        const filename = path.resolve(`${DIST_DIR}/${request.url}`);
        fs.existsSync(filename) && fs.lstatSync(filename).isFile() ? sendFile(filename) : sendFile(`${DIST_DIR}/index.html`);

        function sendFile(filename) {
            response.writeHead(200);
            response.end(fs.readFileSync(filename));
        }
    });

    server.listen(port);
};


export const close = function() {
    server && server.close();
};
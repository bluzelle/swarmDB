const WebSocketServer = require('websocket').server;
const http = require('http');
const nodes = require('./NodeStore').nodes;
const {observable} = require('mobx');
const connections = [];
const CommandProcessors = require('./CommandProcessors');

module.exports.start = (port = 8099) => {
    const httpServer = http.createServer(function (request, response) {

    });

    httpServer.listen(port, () => {
        console.log(`Emulator is listening on port ${port}`);
    });

    const wsServer = new WebSocketServer({
        httpServer: httpServer,
        autoAcceptConnections: true
    });

    wsServer.on('connect', conn => {
        connections.push(conn);
        conn.on('message', ({utf8Data: message}) => {
            const command = JSON.parse(message);
            CommandProcessors[command.cmd](command.data);
        })
    });
};
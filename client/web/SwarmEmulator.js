#!/usr/bin/env node
const TOTAL_NODES = 5;

const _ = require('lodash');
const WebSocketServer = require('websocket').server;
const http = require('http');
const fs = require('fs');
const path = require('path');

_.range(8100, 8100 + TOTAL_NODES).forEach((port) => setTimeout(() => Node(port)));

const nodes = {};

const addNode = (node) => {
    _.map(nodes, n => n.nodeAdded(_.pick(node, 'address', 'ip', 'port')));
    nodes[node.port] = node;
};

function Node(port) {
    const peers = [];

    const me = {
        address: `127.0.0.1:${port}`,
        ip: '127.0.0.1',
        port: port,
        available: 100,
        used: 50,
        nodeAdded: (node) => {
            sendToClients('updateNodes', [node]);
            peers.push(node);
        }
    };

    addNode(me);

    const connections = [];


    const server = http.createServer(function(request, response) {
        const filename = path.resolve(`./dist/${request.url}`);
        fs.existsSync(filename) && fs.lstatSync(filename).isFile() ? sendFile(filename) : sendFile(path.resolve('./dist/index.html'));

        function sendFile(filename) {
            response.writeHead(200);
            response.end(fs.readFileSync(filename));
        }
    });

    server.listen(port, () => {
        console.log(`Node is listening on port ${port}`);
    });

    wsServer = new WebSocketServer({
        httpServer: server,
        autoAcceptConnections: true
    });

    wsServer.on('connect', connection => {
        connections.push(connection);
        sendNodesInfo(connection);
    });

    const sendToClients = (cmd, data) => connections.forEach(connection =>
        sendToClient(connection, cmd, data)
    );

    const sendToClient = (connection, cmd, data) =>
        setTimeout(() => connection.send(JSON.stringify({cmd: cmd, data: data})), 500);

    const sendNodesInfo = (connection) => {
        sendToClient(connection, 'updateNodes', [me, ...peers]);
    };

    (function updateStorageUsed(direction = 1) {
        me.used += direction;
        sendToClients('updateNodes', [_.pick(me, 'ip', 'port', 'address', 'used')]);

        let newDirection = direction;
        me.used < 40  && (newDirection = 1);
        me.used > 70 && (newDirection = -1);
        setTimeout(() => updateStorageUsed(newDirection), 1000);
    }());
}




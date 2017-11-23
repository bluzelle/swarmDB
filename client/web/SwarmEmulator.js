#!/usr/bin/env node
const TOTAL_NODES = 15;

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
    let  alive = true;

    const me = {
        address: `127.0.0.1:${port}`,
        ip: '127.0.0.1',
        port: port,
        available: 100,
        used: _.random(40, 70),
        die: () => {
            alive = false;
            setTimeout(() => alive = true, _.random(10000, 20000));
        },
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
        alive && setTimeout(() => connection.send(JSON.stringify({cmd: cmd, data: data})), 500);

    const sendNodesInfo = (connection) => {
        sendToClient(connection, 'updateNodes', [me, ...peers]);
    };

    (function updateStorageUsed(direction = 1) {
        me.used += direction;
        sendToClients('updateNodes', [_.pick(me, 'ip', 'port', 'address', 'used', 'available')]);

        let newDirection = direction;
        me.used < 40  && (newDirection = _.random(1,2));
        me.used > 70 && (newDirection = _.random(-1, -2));
        setTimeout(() => updateStorageUsed(newDirection), 1000);
    }());
}


(function killANode() {
   setTimeout(() => {
       const keys = Object.keys(nodes);
       const idx = _.random(keys.length - 1);
       const key = keys[idx];
       nodes[key].die();
       killANode();
   },_.random(10000, 5000));

}());




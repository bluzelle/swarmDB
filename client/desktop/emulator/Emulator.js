#!/usr/bin/env node

const _ = require('lodash');
const WebSocketServer = require('websocket').server;
const http = require('http');
const fs = require('fs');
const path = require('path');
const fp = require('lodash/fp');

const nodes = {};

let maxNodes = 15;
let lastPort = 8100;


module.exports = {
    getNodes: () => nodes,
    setMaxNodes: num => maxNodes = num,
    getMaxNodes: () => maxNodes,
    shutdown: () => _.forEach(nodes, node => node.shutdown()),
    start: _.once(start)
};


const addNode = (node) => {
    _.map(nodes, n => n.nodeAdded(_.pick(node, 'address', 'ip', 'port')));
    nodes[node.port] = node;
};


function start() {
    module.exports.wasStarted = true;
    (function checkNeedMoreNodes() {
        _.size(nodes) < maxNodes && Node(lastPort++);
        setTimeout(checkNeedMoreNodes, 500);
    }());

    (function checkNeedLessNodes() {
        _.size(nodes) > maxNodes && getRandomNode().shutdown();
        setTimeout(checkNeedLessNodes, 500);
    }());

    (function killANode() {
        setTimeout(() => {
            getRandomNode().die();
            killANode();
        }, _.random(10000, 5000));
    }());

    (function loop() {
        console.log()
    }());
}


function Node(port) {
    const peers = [];
    let alive = true;

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
        },
        shutdown: () => {
            alive = true;
            me.isShutdown = true;
            delete nodes[port];
            sendToClients('removeNodes', [me.address]);
            setTimeout(() => {
                me.getWsServer().shutDown();
                me.getHttpServer().close();
            }, 200);
        }
    };

    addNode(me);

    const connections = [];


    me.getHttpServer = _.memoize(() => http.createServer(function (request, response) {
        const filename = path.resolve(`./dist/${request.url}`);
        fs.existsSync(filename) && fs.lstatSync(filename).isFile() ? sendFile(filename) : sendFile(path.resolve('./dist/index.html'));

        function sendFile(filename) {
            response.writeHead(200);
            response.end(fs.readFileSync(filename));
        }
    }));

    me.getHttpServer().listen(port, () => {
        console.log(`Node is listening on port ${port}`);
    });

    me.getWsServer = _.memoize(() => new WebSocketServer({
        httpServer: me.getHttpServer(),
        autoAcceptConnections: true
    }));

    me.getWsServer().on('connect', connection => {
        connections.push(connection);
        sendNodesInfo(connection);
    });

    const sendToClients = (cmd, data) => connections.forEach(connection =>
        sendToClient(connection, cmd, data)
    );

    const sendToClient = (connection, cmd, data) =>
        alive && connection.send(JSON.stringify({cmd: cmd, data: data}));

    const sendNodesInfo = (connection) => {
        sendToClient(connection, 'updateNodes', [me, ...peers]);
    };

    (function updateStorageUsed(direction = 1) {
        me.used += direction;
        sendToClients('updateNodes', [_.pick(me, 'ip', 'port', 'address', 'used', 'available')]);

        let newDirection = direction;
        me.used < 40 && (newDirection = _.random(1, 2));
        me.used > 70 && (newDirection = _.random(-1, -2));
        setTimeout(() => me.isShutdown || updateStorageUsed(newDirection), 1000);
    }());

    (function sendMessage() {
        setTimeout(() => {
            me.isShutdown || sendToClients('messages', [
                {
                    srcAddr: getOtherRandomNode().address,
                    timestamp: new Date().getTime(),
                    body: {something: `sent - ${_.uniqueId()}`}
                }
            ]);
            me.isShutdown || sendMessage();
        }, _.random(5000, 10000));
    }());
}

const getRandomNode = fp.pipe(
    () => _.random(_.size(nodes) - 1),
    idx => nodes[Object.keys(nodes)[idx]],
);

const getOtherRandomNode = node => {
    const n = getRandomNode();
    return n === node ? getOtherRandomNode() : n;
};







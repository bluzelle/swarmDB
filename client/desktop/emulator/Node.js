const WebSocketServer = require('websocket').server;
const http = require('http');
const fs = require('fs');
const path = require('path');
const _ = require('lodash');
const fp = require('lodash/fp');
const nodes = require('./NodesService').nodes;

module.exports = function Node(port) {
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
        },
        shutdown: () => {
            nodes.delete(port);
            sendToClients('removeNodes', [me.address]);
            alive = false;
            setTimeout(() => {
                me.getWsServer().shutDown();
                me.getHttpServer().close();
            }, 200);
        }
    };

    nodes.set(me.port, me);

    const connections = [];


    const DIST_DIR = path.resolve('../web/dist');
    me.getHttpServer = _.memoize(() => http.createServer(function (request, response) {

        const filename = path.resolve(`${DIST_DIR}/${request.url}`);
        fs.existsSync(filename) && fs.lstatSync(filename).isFile() ? sendFile(filename) : sendFile(`${DIST_DIR}/index.html`);

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
        alive && sendToClient(connection, 'updateNodes', [me, ...getPeerInfo(me)]);
    };

    nodes.observe(() => {
        alive && connections.forEach(sendNodesInfo);
    });



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

    function getPeerInfo(node) {
        return nodes.values().filter(peer => peer.address !== node.address).map(n => _.pick(n, 'address', 'ip', 'port'));
    }

    const getOtherRandomNode = () => {
        const n = getRandomNode();
        return n === me ? getOtherRandomNode() : n;
    };

    const getRandomNode = fp.pipe(
        () => _.random(nodes.size - 1),
        idx => nodes.values()[idx],
    );
};

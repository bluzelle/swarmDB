const WebSocketServer = require('websocket').server;
const http = require('http');
const fs = require('fs');
const path = require('path');
const _ = require('lodash');
const fp = require('lodash/fp');
const nodes = require('./NodeStore').nodes;
const {behaveRandomly} = require('./Values');

module.exports = function Node(port) {
    let alive = true;

    nodes.set(port, {
        address: `127.0.0.1:${port}`,
        ip: '127.0.0.1',
        port: port,
        available: 100,
        used: _.random(40, 70),
        isLeader: false,
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
    });

    const me = nodes.get(port);


    const connections = [];

    const DIST_DIR = path.resolve(__dirname, '../web/dist');
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

    (function becomeOrDropLeader() {
       if(behaveRandomly.get()) {
           me.isLeader === true ? dropLeader() : (noLeaderExists() && becomeLeader());

           function dropLeader() {
               me.isLeader = false;
               sendIsLeaderToClients();
           }

           function becomeLeader() {
               me.isLeader = true;
               sendIsLeaderToClients();
           }

           function noLeaderExists() {
               return nodes.values().some(n => n.isLeader) === false;
           }
       }
       setTimeout(becomeOrDropLeader, me.isLeader ? 10000 : 2000);

        function sendIsLeaderToClients() {
           sendToClients('updateNodes', [_.pick(me, 'ip', 'port', 'address', 'isLeader')])
        }
    }());

        (function updateStorageUsed(direction = 1) {
            if(behaveRandomly.get()) {
                me.used += direction;
                sendToClients('updateNodes', [_.pick(me, 'ip', 'port', 'address', 'used', 'available')]);

                me.used < 40 && (direction = _.random(1, 2));
                me.used > 70 && (direction = _.random(-1, -2));
            }
            setTimeout(() => me.isShutdown || updateStorageUsed(direction), 1000);
        }());

        (function sendMessage() {
            setTimeout(() => {
                if(behaveRandomly.get() && nodes.size > 1) {
                    me.isShutdown || sendToClients('messages', [
                        {
                            srcAddr: getOtherRandomNode().address,
                            timestamp: new Date().getTime(),
                            body: {something: `sent - ${_.uniqueId()}`}
                        }
                    ]);
                }
                me.isShutdown || sendMessage();
            }, _.random(5000, 10000));
        }());

    function getPeerInfo(node) {
        return nodes.values().filter(peer => peer.address !== node.address).map(n => _.pick(n, 'address', 'ip', 'port'));
    }

    const getOtherRandomNode = () => {
        const n = getRandomNode();
        return n.address === me.address ? getOtherRandomNode() : n;
    };

    const getRandomNode = fp.pipe(
        () => _.random(nodes.size - 1),
        idx => nodes.values()[idx],
    );
    return me;
};

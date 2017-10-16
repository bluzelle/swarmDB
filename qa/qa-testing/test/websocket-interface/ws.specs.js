const WebSocket = require('ws');
const waitUntil = require('async-wait-until');
const getProp = require('lodash/get');

let socket, messages;

describe('web sockets interface', () => {

    beforeEach((done) => {
        messages = [];
        socket = new WebSocket('ws://localhost:8000');
        socket.on('open', done);
        socket.on('message', message => messages.push(JSON.parse(message)));

    });

    describe('getNodes', () => {
        it('should result in a addNodes command', async () => {
            wsSend('getAllNodes');
            await waitUntil(() => getProp(messages, '[0].cmd') === 'addNodes');
        })
    });

    afterEach(() => socket.close());
});

const wsSend = (cmd, data) => {
    socket.send(JSON.stringify({cmd: cmd, seq: seq(), data: data}));
};

const seq = ((num) => () => num++)(0);
import WebSocket from 'ws';
import waitUntil from 'async-wait-until';
import {logFileExists, logFileMoved} from '../utils.js';
import {get} from 'lodash';
import {exec}  from 'child_process';

let socket, messages, logFileName;

describe('web sockets interface', () => {

    beforeEach( async done => {
        exec('cd ../../; yarn run-daemon');
        await waitUntil(() => logFileName = logFileExists());

        messages = [];
        socket = new WebSocket('ws://localhost:49152');
        socket.on('open', done);
        socket.on('message', message => messages.push(JSON.parse(message)));
    });

    describe('ping', () => {
        it('should respond with pong', async () => {
            socket.send('{ "bzn-api" : "ping" }');
            await waitUntil(() => get(messages, '[0].bzn-api') === 'pong');
        })
    });

    afterEach( async () => {
        socket.close();
        exec('cd ../../; yarn daemon-kill');
        await waitUntil( () => logFileMoved(logFileName));
    });
});

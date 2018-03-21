import waitUntil from 'async-wait-until';
import {logFileExists, logFileMoved, readFile} from '../utils.js';
import {exec}  from 'child_process';

let logFileName;

describe('daemon', () => {

    beforeEach( async () => {
        exec('cd ../../; yarn run-daemon'); // Working directory originates to where Chimp is called)
        await waitUntil( () => logFileName = logFileExists());

        exec('cd ../../; yarn daemon-kill');
        await waitUntil( () => logFileMoved(logFileName));
    });

    describe('on startup', () => {
        it('should create a log', () =>
            expect(readFile('/daemon/logs/', logFileName)).to.have.string('Loading: bluzelle.json')
        );
    });

    describe('on shutdown', () => {
       it('should log "shutting down"', () =>
           expect(readFile('/daemon/logs/', logFileName)).to.have.string('signal received -- shutting down')
       );
    });

    afterEach(() => logFileName = '');
});

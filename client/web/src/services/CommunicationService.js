import curry from 'lodash/fp/curry'


let socket;
let seq = 0;
const commandProcessors = [];

const RETRY_TIME = 1000;

export const socketState = observable('closed');
export const daemonUrl = observable(undefined);
export const closeCode = observable(undefined);
export const disconnect = () => {
    daemonUrl.set(undefined);
    socket.close();
};

autorun(() => daemonUrl.get() && startSocket(daemonUrl.get()));



const setSocketState = () => socketState.set(socketStates[socket.readyState]);

export const receiveMessage = (data) => {
    const msg = JSON.parse(data);
    commandProcessors[msg.cmd] ? commandProcessors[msg.cmd](msg.data) : console.error(`${msg.cmd} has no command processor`)
};

const startSocket = (url) => {
    socket = new WebSocket(`ws://${url}`);
    setSocketState();
    closeCode.set(undefined);

    socket.onopen = setSocketState;

    socket.onmessage = (ev) => receiveMessage(ev.data);

    socket.onclose = (ev) => {
        // the code according to https://tools.ietf.org/html/rfc6455#section-11.7
        closeCode.set(ev.code);
        setSocketState();
        setTimeout(() => daemonUrl.get() && startSocket(), RETRY_TIME);
    };

    socket.onerror = (ev) => {
        daemonUrl.set(undefined);
        setSocketState();
    };
};

export const sendCommand = curry((cmd, data) => {
    socket && socket.readyState === WebSocket.OPEN ? (
        socket.send(JSON.stringify({cmd: cmd, data: data, seq: seq++}))
    ) : (
        setTimeout(() => sendCommand(cmd, data), 100)
    )
});

export const addCommandProcessor = (name, fn) => commandProcessors[name] = fn;

const socketStates = ['opening', 'open', 'closing', 'closed'];
import curry from 'lodash/fp/curry'
import {getNodes} from 'services/NodeService'
import groupBy from 'lodash/groupBy'

const commandProcessors = [];
const MIN_CONNECTED_NODES = 3000;

export const entryPointUrl = observable(undefined);
const entryPointSocket = observable(undefined);

autorun(function checkConnectToEntryPoint() {
    if(entryPointUrl.get()) {
        const [ip, port] = entryPointUrl.get().split(':');
        connectToEntryPoint(ip, port);
    }
});

autorun(function checkConnectedToNode() {
    const nodeOpen = getNodes().some(node => node.socketState === 'open');
    entryPointSocket.get() && nodeOpen && entryPointSocket.get().close()
});

autorun(function checkNeedToConnectToNode() {
        const nodes = getNodes();
        const groups = Object.assign(
            {connected: [], notConnected: []},
            groupBy(nodes, node => node.socket ? 'connected' : 'notConnected')
        );
        if(groups.connected.length < MIN_CONNECTED_NODES && groups.notConnected.length) {
            connectToNode(groups.notConnected[0]);
            setTimeout(checkNeedToConnectToNode());
        }
    }
);

const connectToEntryPoint = (ip, port) => {
    const socket = new WebSocket(`ws://${ip}:${port}`);
    entryPointSocket.set(socket);
    socket.onmessage = ev => receiveMessage(ev.data);
    socket.onerror = () => {
        entryPointSocket.get().close();
        entryPointSocket.set(undefined);
    };
    socket.onclose = () => entryPointSocket.set(undefined);
};

export const connectToNode = (node) => {
    const socket = new WebSocket(`ws://${node.ip}:${node.port}`);
    node.socket = socket;
    setSocketState();
    socket.onopen = setSocketState;
    socket.onmessage = (ev) => receiveMessage(ev.data);
    socket.onclose = () => {
        setSocketState();
        node.socket = undefined;
    };
    socket.onerror = () => socket.close();

    function setSocketState() {
        node.socketState ? node.socketState = socketStates[socket.readyState] :
        extendObservable(node, {socketState: socketStates[socket.readyState]});
    }
};

export const receiveMessage = (data) => {
    const msg = JSON.parse(data);
    commandProcessors[msg.cmd] ? commandProcessors[msg.cmd](msg.data) : console.error(`${msg.cmd} has no command processor`)
};


export const addCommandProcessor = (name, fn) => commandProcessors[name] = fn;

const socketStates = ['opening', 'open', 'closing', 'closed'];

// --------------------------------------


let socket;
let seq = 0;

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


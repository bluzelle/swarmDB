import curry from 'lodash/fp/curry'
import {getNodes} from 'services/NodeService'
import groupBy from 'lodash/groupBy'
import {tick} from 'services/TickService'

const commandProcessors = [];
const MIN_CONNECTED_NODES = 3000;

export const entryPointUrl = observable(undefined);
const entryPointSocket = observable(undefined);

autorun(function checkConnectToEntryPoint() {
    if (entryPointUrl.get()) {
        const [ip, port] = entryPointUrl.get().split(':');
        connectToEntryPoint(ip, port);
    }
});

autorun(function checkConnectedToNode() {
    const nodeOpen = getNodes().some(node => node.socketState === 'open');
    entryPointSocket.get() && nodeOpen && entryPointSocket.get().close()
});

autorun(function checkNeedToConnectToNode() {
        tick.get();
        untracked(() => {
            const nodes = getNodes();
            const groups = Object.assign(
                {connected: [], notConnected: []},
                groupBy(nodes, node => node.socket ? 'connected' : 'notConnected')
            );
            if (groups.connected.length < MIN_CONNECTED_NODES && groups.notConnected.length) {
                connectToNode(groups.notConnected[0]);
//                setTimeout(checkNeedToConnectToNode, 1000);
            }
        });
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

const connectToNode = (node) => {
    const socket = new WebSocket(`ws://${node.ip}:${node.port}`);
    node.socket = socket;
    setSocketState();
    socket.onopen = setSocketState;
    socket.onmessage = (ev) => receiveMessage(ev.data, node);
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

export const receiveMessage = (data, node) => {
    const msg = JSON.parse(data);
    commandProcessors[msg.cmd] ? commandProcessors[msg.cmd](msg.data, node) : console.error(`${msg.cmd} has no command processor`)
};


export const addCommandProcessor = (name, fn) => commandProcessors[name] = fn;

const socketStates = ['opening', 'open', 'closing', 'closed'];



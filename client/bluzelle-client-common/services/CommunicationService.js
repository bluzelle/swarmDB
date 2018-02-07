import {getNodes} from './NodeService'
import groupBy from 'lodash/groupBy'
import {receiveMessage} from './CommandService'

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
    entryPointSocket.get() && nodeOpen && entryPointSocket.get().close();
});

(function checkConnectToNode() {
    const nodes = getNodes();
    const groups = Object.assign(
        {connected: [], notConnected: []},
        groupBy(nodes, node => node.socket ? 'connected' : 'notConnected')
    );
    if (groups.connected.length < MIN_CONNECTED_NODES && groups.notConnected.length) {
        connectToNode(groups.notConnected[0]);
    }
    setTimeout(checkConnectToNode, 250);
}());


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

function connectToNode(node) {
    const socket = new WebSocket(`ws://${node.ip}:${node.port}`);
    node.socket = socket;
    setSocketState();
    socket.onopen = setSocketState;
    socket.onmessage = (ev) => receiveMessage(ev.data, node);
    socket.onclose = () => {
        setSocketState();
        node.socket = undefined;
    };
    socket.onerror = () => {
        socket.close();
        setSocketState();
        node.socket = undefined;
    };

    function setSocketState() {
        extendObservable(node, {socketState: socketStates[socket.readyState]});
    }
}


export const sendToNodes = (cmd, data) => getNodes().forEach(node =>
    sendToNode(node, cmd, data));

export const sendToNode = (node, cmd, data) =>
    node.socketState === 'open' && node.socket.send(JSON.stringify({cmd, data}));



const socketStates = ['opening', 'open', 'closing', 'closed'];
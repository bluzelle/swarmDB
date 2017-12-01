import {sendCommand, addCommandProcessor} from 'services/CommunicationService'
import {socketState} from 'services/CommunicationService'
import {transactionBundler} from "../Utils"

const nodes = observable.map({});

global.nodes = nodes;

setTimeout(() => {
    addCommandProcessor('updateNodes', (newNodes) => newNodes.forEach(updateNode));
    addCommandProcessor('removeNodes', (addresses) => addresses.forEach(removeNodeByAddress));
});


export const getNodes = () => nodes.values();
export const getNodeByAddress = address => nodes.get(address);
export const getNodeByName = name => node.get(name);

export const updateNode = transactionBundler('updateNode', nodeInfo => {
    const foundNode = nodes.get(`${nodeInfo.ip}:${nodeInfo.port}`);
    nodeInfo.lastUpdate = new Date().getTime();
    foundNode ? extendObservable(foundNode, nodeInfo) : addNewNode(nodeInfo);
});

export const removeNodeByAddress = transactionBundler('removeNodeByAddress', address => {
    nodes.delete(address);
});

(function checkNodeAlive() {
    nodes.values().map(node =>
        node.socket &&
        node.socketState === 'open' &&
        new Date().getTime() - node.createdAt > 3000 &&
        markNodeAlive(node.address)
    );
    setTimeout(checkNodeAlive, 1000);
}());

const markNodeAlive = transactionBundler('markNodeAlive', name => {
    Maybe.fromNull(nodes.get(name))
        .map(n => n.nodeState = 'alive');
});

const addNewNode = node => {
    const newNode = {nodeState: 'new', createdAt: new Date().getTime(), socketState: undefined, ...node};
    nodes.set(`${node.ip}:${node.port}`, newNode);
};

export const markNodeDead = action(node => {
        node.nodeState = 'dead';
});

export const clearNodes = () => nodes.clear();


(function checkForDeadNodes() {
    nodes.values().forEach(node =>  new Date().getTime() - node.createdAt > 3000 && node.nodeState !== 'dead' && (node.socket || markNodeDead(node)));
    setTimeout(checkForDeadNodes, 1000);
}());


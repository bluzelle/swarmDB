import {sendCommand, addCommandProcessor} from 'services/CommunicationService'
import {socketState} from 'services/CommunicationService'
import {transactionBundler} from "../Utils"

const nodes = observable.map({});

setTimeout(() => {
    addCommandProcessor('updateNodes', (newNodes) => newNodes.forEach(updateNode));
//    addCommandProcessor('removeNodes', (addresses) => addresses.forEach(removeNodeByAddress));
});


export const getNodes = () => nodes.values();
export const getNodeByAddress = address => nodes.get(address);
export const getNodeByName = name => node.get(name);

export const updateNode = transactionBundler('updateNode', nodeInfo => {
    const foundNode = nodes.get(`${nodeInfo.ip}:${nodeInfo.port}`);
    nodeInfo.lastUpdate = new Date().getTime();
    foundNode && foundNode.nodeState === 'dead' && (foundNode.nodeState = 'alive');
    foundNode && foundNode.nodeState === 'new' && new Date().getTime() - foundNode.createdAt > 3000 && (nodeInfo.nodeState = 'alive');
    foundNode ? extendObservable(foundNode, nodeInfo) : addNewNode(nodeInfo);
});

const markNodeAlive = transactionBundler('markNodeAlive', name => {
    Maybe.fromNull(nodes.get(name))
        .map(n => n.nodeState = 'alive');
});

const addNewNode = node => {
    const newNode = {nodeState: 'new', createdAt: new Date().getTime(), ...node};
    nodes.set(`${node.ip}:${node.port}`, newNode);
};

export const markNodeDead = action(node => {
        node.nodeState = 'dead';
        node.socket && node.socket.close();
        node.socket = undefined;
});

export const clearNodes = () => nodes.clear();

const isNodeAlive = node => new Date().getTime() - node.lastUpdate < 5000;

(function checkForDeadNodes() {
    nodes.values().forEach(node =>  node.nodeState !== 'dead' && (isNodeAlive(node) || markNodeDead(node)));
    setTimeout(checkForDeadNodes, 1000);
}());


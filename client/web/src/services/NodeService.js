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

export const updateNode = transactionBundler('updateNode', node => {
    const foundNode = nodes.get(`${node.ip}:${node.port}`);
    foundNode ? extendObservable(foundNode, node) : addNewNode(node);
});

const markNodeAlive = transactionBundler('markNodeAlive', name => {
    Maybe.fromNull(nodes.get(name))
        .map(n => n.nodeState = 'alive');
});

const addNewNode = node => {
    const newNode = {nodeState: 'new', ...node};
    nodes.set(`${node.ip}:${node.port}`, newNode);
    setTimeout(() => markNodeAlive(`${node.ip}:${node.port}`) ,3000);
};

export const removeNodeByAddress = action(address => {
    const found = nodes.get(address);
    found && (found.nodeState = 'dead');
    setTimeout(() => {
        nodes.delete(address);
    }, 2000);
});

export const clearNodes = () => nodes.clear();


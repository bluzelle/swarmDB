import {sendCommand, addCommandProcessor} from 'services/CommunicationService'
import {socketState} from 'services/CommunicationService'
import {transactionBundler} from "../Utils"

global.observable = observable;
global.autorun = autorun;
global.observer = observer;
global.extendObservable = extendObservable;

const nodes = observable.map({});

global.nodes = nodes;

addCommandProcessor('updateNodes', (newNodes) => newNodes.forEach(updateNode));
addCommandProcessor('removeNodes', (addresses) => addresses.forEach(removeNodeByAddress));

autorun(() => socketState.get() === 'open' && untracked(resetNodes));

const resetNodes = () => {
    sendCommand("getAllNodes", undefined);
    clearNodes();
};

export const getNodes = () => nodes.values();
export const getNodeByAddress = address => nodes.get(address);

export const updateNode = transactionBundler('updateNode', node => {
    const foundNode = nodes.get(node.address);
    foundNode ? extendObservable(foundNode, node) : addNewNode(node);
});

const markNodeAlive = transactionBundler('markNodeAlive', address => {
    Maybe.fromNull(nodes.get(address))
        .map(n => n.nodeState = 'alive');
});

const addNewNode = node => {
    const newNode = {nodeState: 'new', ...node};
    nodes.set(node.address, newNode);
    setTimeout(() => markNodeAlive(newNode.address) ,3000);
};

export const removeNodeByAddress = action(address => {
    const found = nodes.get(address);
    found && (found.nodeState = 'dead');
    setTimeout(() => {
        nodes.delete(address);
    }, 2000);
});

export const clearNodes = () => nodes.clear();


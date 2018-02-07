import {addCommandProcessor} from "./CommandService";
import {getNodes} from './NodeService';

export const log = ({level, message, node}) => logEntries.push({
    timestamp: new Date().getTime(),
    level,
    message,
    node
});

const logEntries = observable([]);

setTimeout(() => {
    addCommandProcessor('log', action('log', entries => entries.forEach(l => logEntries.push(l))));
});

export const getLogEntries = () => logEntries;

// ----------- watchers ----------------
let lastDeadNodeAddresses = [];
const knownNodeAddresses = [];


autorun(function checkRecoveredNodes() {
    getNodes().forEach(node => {
        node.nodeState === 'alive' && lastDeadNodeAddresses.includes(node.address) && log({
            level: 'info',
            message: 'Node recovered',
            node
        })

    });
});

autorun(function checkDeadNodes() {
    const deadNodes = getNodes().filter(node => node.nodeState === 'dead');
    deadNodes.forEach(node => lastDeadNodeAddresses.includes(node.address) || log({
        level: 'severe',
        message: 'Node marked dead',
        node
    }));
    lastDeadNodeAddresses = deadNodes.map(n => n.address);
});

autorun(function checkNewNodes() {
    const newNodes = getNodes().forEach(node => {
        knownNodeAddresses.includes(node.address) === false &&
        log({
            level: 'info',
            message: 'Node added',
            node
        });
        knownNodeAddresses.push(node.address);
    })
});


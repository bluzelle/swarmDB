#!/usr/bin/env node
const Node = require('./Node');
const fp = require('lodash/fp');

require.main === module && setTimeout(start);


const {observable} = require('mobx');

const _ = require('lodash');

const nodes = require('./NodesService').nodes;
const maxNodes = observable(5);
let lastPort = 8100;

module.exports = {
    getNodes: () => nodes.values(),
    setMaxNodes: num => maxNodes.set(num),
    getMaxNodes: () => maxNodes.get(),
    shutdown: () => nodes.values().map(node => node.shutdown()),
    start: _.once(start)
};




function start() {
    module.exports.wasStarted = true;

    (function checkNeedMoreNodes() {
        nodes.size < maxNodes.get() && Node(lastPort++);
        setTimeout(checkNeedMoreNodes, 250);
    }());

    (function checkNeedLessNodes() {
        nodes.size > maxNodes.get() && getRandomNode().shutdown();
        setTimeout(checkNeedLessNodes, 250);
    }())
}



const getRandomNode = fp.pipe(
    () => _.random(nodes.size - 1),
    idx => nodes.values()[idx],
);








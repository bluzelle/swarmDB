#!/usr/bin/env node
const _ = require('lodash');
const Node = require('./Node');
const fp = require('lodash/fp');
const {maxNodes, behaveRandomly} = require('./Values');
const CommandProcessors = require('./CommandProcessors');
const {getData, setData} = require('./DataStore.js');

require.main === module && setTimeout(start);


const nodes = require('./NodeStore').nodes;
let lastPort;
let randomBehavior = false;

module.exports = {
    getNodes: () => nodes.values(),
    setMaxNodes: CommandProcessors.setMaxNodes,
    getMaxNodes: () => maxNodes.get(),
    shutdown: () => nodes.values().map(node => node.shutdown()),
    start: _.once(start),
    getData: getData,
    setData: setData,
    behaveRandomly: (v) => behaveRandomly.set(v),
    isRandom: () => behaveRandomly.get()
};




function start(startPort = 8100) {
    require('./Network').start(startPort - 1);

    lastPort = startPort;
    module.exports.wasStarted = true;

    (function checkNeedMoreNodes() {
        if(nodes.size < maxNodes.get()) {
            const node = Node(lastPort++);
            randomBehavior && node.behaveRandomly();
        }
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








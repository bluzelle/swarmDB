#!/usr/bin/env node
const _ = require('lodash');
const Node = require('./Node');
const fp = require('lodash/fp');
const {maxNodes, behaveRandomly} = require('./Values');
const CommandProcessors = require('./CommandProcessors');
const {getData, setData} = require('./DataStore.js');

require.main === module && setTimeout(start);


const nodes = require('./NodeStore').nodes;
let initialStartPort;
let lastPort;
let randomBehavior = false;

module.exports = {
    getNodes: () => nodes.values(),
    setMaxNodes: CommandProcessors.setMaxNodes,
    getMaxNodes: () => maxNodes.get(),
    shutdown: () => {
        maxNodes.set(0);
        lastPort = initialStartPort;
        return nodes.values().map(node => node.shutdown())
    },
    start: _.once(start),
    getData: getData,
    setData: setData,
    behaveRandomly: (v) => behaveRandomly.set(v),
    isRandom: () => behaveRandomly.get()
};

module.exports.reset = async function() {

    this.start();

    await Promise.all(this.shutdown());

    this.setMaxNodes(1);
    setData({});

    await new Promise(resolve => (function loop() {

        nodes.keys().length 
            ? resolve()
            : setTimeout(loop, 0);

    })());

}.bind(module.exports);




function start(startPort = 8100) {
    initialStartPort = lastPort = startPort;
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








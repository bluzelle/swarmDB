const {observable} = require('mobx');
const _ = require('lodash');

const nodes = observable.map({});

module.exports = {
    nodes: nodes,
    getAllNodesInfo: () =>
        nodes.values().map(n => _.pick(n, 'address', 'ip', 'port', 'used', 'available'))
};

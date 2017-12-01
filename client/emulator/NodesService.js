const {observable} = require('mobx');

const nodes = observable.map({});

module.exports = {
    nodes: nodes
};
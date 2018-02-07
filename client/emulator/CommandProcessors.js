const {maxNodes} = require('./Values');
const {sendChangesToNode, requestBytearray, requestKeyList} = require('./DataStore');

module.exports = {
    setMaxNodes: num => maxNodes.set(num),
    sendChangesToNode,
    requestBytearray,
    requestKeyList
};
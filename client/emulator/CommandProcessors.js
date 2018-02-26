const {maxNodes} = require('./Values');
const {sendDataToNode, requestBytearray, requestKeyList} = require('./DataStore');
const {getAllNodesInfo} = require('./NodeStore.js');

module.exports = {
    setMaxNodes: num => maxNodes.set(num),
    requestAllNodes: (data, connection) =>
        connection.send(JSON.stringify({cmd: 'updateNodes', data: getAllNodesInfo()})),
    sendDataToNode,
    requestBytearray,
    requestKeyList
};
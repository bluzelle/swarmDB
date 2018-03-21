const _ = require('lodash');
const request = require('request');

export const sendCommand = (cmd, data) => {
    const q = encodeURIComponent(JSON.stringify({cmd: cmd, data:data}));
    request(`http://localhost:3002/functional/api?q=${q}`, function(error, response, body) {
        error && console.log('Received error sending command to echo service', error);
    });
};

export const addNodes = (num = 1) => {
      sendCommand('addNodes', num);
};

export const addNode = (data = {}) => {
    const nodeInfo = {address: `0x99999999999-${_.uniqueId()}`, ...data};
    sendCommand('updateNodes', [nodeInfo]);
    return nodeInfo;
};

export const updateNode = (address, data = {}) => {
    sendCommand('updateNodes', [{address: address, ...data}]);
};

export const sendLogMessage = (message) => {
    sendCommand('log', {timestamp: 'some time here', timer_no: 1, entry_no: _.uniqueId(), message: message});
};


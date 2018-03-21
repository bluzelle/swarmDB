const {observable, toJS, observe} = require('mobx');
const {forEach} = require('lodash');
const {nodes} = require('./NodeStore');


const data = observable.map({});

module.exports = {

    requestKeyList: (obj, ws) => {
        var response = [];

        data.keys().forEach( key =>
            response.push({cmd: 'update', data: {key: key}})
        );

        ws.send(JSON.stringify(
            {
                cmd: 'aggregate',
                data: response
            }
        ))
    },

    read: (obj, ws) => {
        ws.send(JSON.stringify(
            {
                cmd: 'update',
                data:
                    {
                        key: obj.key,
                        bytearray: data.get(obj.key)
                    }
            }
        ))
    },

    update: obj => {
        data.set(obj.key, obj.bytearray);
    },

    destroy: obj => {
        data.delete(obj.key);
    },

    getData: () => data,
    setData: obj => {
        data.clear();
        data.merge(obj);
    }
};


observe(data, (changes) => {
    nodes.forEach(node => node.sendToClients({
        cmd: changes.type === 'delete' ? 'delete' : 'update',
        data: {key: changes.name}
    }));
});
const {observable, toJS, observe} = require('mobx');
const {forEach} = require('lodash');
const {nodes} = require('./NodeStore');


const data = observable.map(getSampleData());

const mergeAndDelete = (val, key) =>
    val === 'deleted' ? data.delete(key) : data.set(key, val);

module.exports = {

    requestBytearray: (obj, ws) =>
        ws.send(JSON.stringify(
            {
                cmd: 'bytearrayUpdate',
                data: {
                    key: obj.key,
                    bytearray: data.get(obj.key)
                }
            })),

    requestKeyList: (obj, ws) =>
        ws.send(JSON.stringify(
            {
                cmd: 'keyListUpdate',
                data: data.keys()
            })),

    sendDataToNode: changes => forEach(changes, mergeAndDelete),

    getData: () => data,
    setData: obj => {
        data.clear();
        data.merge(obj);
    }
};


observe(data, (changes) => {
    nodes.forEach(node => node.sendToClients({
        cmd: changes.type === 'delete' ? 'keyListDelete' : 'keyListUpdate',
        data: [changes.name]
    }));
});


function getSampleData() {
    return {
        someText: [1, 72, 101, 108, 108, 111, 32, 119, 111, 114, 108, 100, 44, 32, 116, 104, 105, 115, 32, 105, 115, 32, 115, 111, 109, 101, 32, 112, 108, 97, 105, 110, 32, 116, 101, 120, 116, 46],
        helloWorldObj: [0, 123, 34, 99, 111, 119, 34, 58, 34, 109, 111, 111, 34, 125],
        complexObject: [0, 123, 34, 97, 114, 114, 97, 121, 115, 34, 58, 91, 49, 44, 50, 44, 91, 123, 34, 102, 105, 101, 108, 100, 34, 58, 34, 102, 101, 105, 108, 100, 34, 125, 44, 91, 93, 93, 44, 51, 44, 91, 34, 97, 112, 112, 108, 101, 115, 34, 44, 91, 34, 97, 110, 100, 34, 44, 91, 34, 111, 114, 97, 110, 103, 101, 115, 34, 93, 93, 93, 93, 125],
        anotherKey: [0, 123, 34, 102, 105, 101, 108, 100, 65, 34, 58, 49, 46, 50, 51, 44, 34, 102, 105, 101, 108, 100, 66, 34, 58, 52, 46, 53, 54, 44, 34, 99, 114, 97, 122, 121, 79, 98, 106, 101, 99, 116, 34, 58, 123, 34, 116, 114, 117, 101, 34, 58, 102, 97, 108, 115, 101, 125, 44, 34, 98, 111, 111, 108, 34, 58, 116, 114, 117, 101, 125]
    };
}
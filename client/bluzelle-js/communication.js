const WebSocket = require('isomorphic-ws');

const connections = new Set();
const resolvers = new Map();
let uuid;

const ping = () => new Promise(resolve => {

    send({
        cmd: 'ping',
        'bzn-api': 'ping'
    }, obj => resolve());

});


const connect = (addr, id) => {
    uuid = id;

    return new Promise(resolve => {

        const s = new WebSocket(addr);

        s.onopen = () => {

            connections.add(s);
            resolve(s);

        };

        s.onclose = () => connections.delete(s);
        s.onerror = e =>  {

            s.close();
            console.error(e);

        }

        s.onmessage = e =>
            onMessage(JSON.parse(e.data), s)

    });

};


const onMessage = (event, socket) => {

    if(event.response_to === undefined) {

        throw new Error('Received non-response message.');

    }

    resolvers.get(event.response_to)(event);

};


// TODO: return promise

const disconnect = () => {
    for(let connection of connections.values()) {
        connection.close();
    }
};


const amendBznApi = obj =>
    Object.assign(obj, {
        'bzn-api': 'crud'
    });

const amendUuid = (uuid, obj) =>
    Object.assign(obj, {
        'db-uuid': uuid
    });


const amendRequestID = (() => {

    let requestIDCounter = 0;

    return obj =>
        Object.assign(obj, {
            'request_id': requestIDCounter++
        });

})();


const send = (obj, resolver) => {

    const message = amendUuid(uuid , amendRequestID(obj));
    resolvers.set(message.request_id, resolver);

    for(let connection of connections.values()) {
        connection.send(JSON.stringify(message));
    }
};

const setup = () => new Promise((resolve, reject) => {

    const cmd = amendBznApi({
        cmd: 'setup'
    });


    send(cmd, obj =>
        obj.error ? reject(new Error(obj.error)) : resolve());

});


const update = (key, value) => new Promise((resolve, reject) => {

    const cmd = amendBznApi({
        cmd: 'update',
        data: {
            key, value
        }
    });

    send(cmd, obj =>
        obj.error ? reject(new Error(obj.error)) : resolve());

});


const remove = key => new Promise((resolve, reject) => {

    const cmd = amendBznApi({
        cmd: 'delete',
        data: {
            key
        }
    });


    send(cmd, obj =>
        obj.error ? reject(new Error(obj.error)) : resolve());

});


const read = key => new Promise((resolve, reject) => {

    const cmd = amendBznApi({
        cmd: 'read',
        data: {
            key
        }
    });


    send(cmd, obj =>
        obj.error ? reject(new Error(obj.error)) : resolve(obj.data.value));

});


const has = key => new Promise(resolve => {

    const cmd = amendBznApi({
        cmd: 'has',
        data: {
            key
        }
    });


    send(cmd, obj => resolve(obj.data.value));

});


const keys = () => new Promise(resolve => {

    const cmd = amendBznApi({
        cmd: 'keys'
    });

    send(cmd, obj => resolve(obj.data.value));

});



module.exports = {
    getUuid: () => uuid,
    connect,
    disconnect,
    ping,
    setup,
    read,
    update,
    remove,
    has,
    keys

};



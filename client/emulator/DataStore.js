const {observable, toJS, observe} = require('mobx');
const {forEach} = require('lodash');
const {nodes} = require('./NodeStore');


const data = observable.map({});

module.exports = {

    read: ({request_id, data:{key}}, ws) => {

        if(data.has(key)) {

            ws.send(JSON.stringify(
                {
                    cmd: 'update',
                    data:
                        {
                            key,
                            value: data.get(key)
                        },
                    response_to: request_id
                }
            ));

        } else {

            ws.send(JSON.stringify(
                {
                    error: `Key "${key}" not in database.`,
                    response_to: request_id
                }
            ));

        }
    },

    update: ({request_id, data:{key, value}}, ws) => {
        data.set(key, value);

        ws.send(JSON.stringify(
            {
                response_to: request_id
            }
        ));
    },

    has: ({request_id, data:{key}}, ws) => {
        ws.send(JSON.stringify(
            {
                data: 
                    {
                        value: data.has(key)
                    },
                response_to: request_id
            }
        ));
    },

    'delete': ({request_id, data:{key}}, ws) => {

        if(data.has(key)) {

            data.delete(key);

            ws.send(JSON.stringify(
                {
                    response_to: request_id
                }
            ));

        } else {

            ws.send(JSON.stringify(
                {
                    error: `Key "${key}" not in database.`,
                    response_to: request_id
                }
            ));

        }

    },

    keys: ({request_id}, ws) => {

        ws.send(JSON.stringify(
            {
                data: {
                    value: data.keys()
                },
                response_to: request_id
            }
        ));

    },

    getData: () => data,
    setData: obj => {
        data.clear();
        data.merge(obj);
    }
};
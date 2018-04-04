const {observable, toJS, observe} = require('mobx');
const {forEach} = require('lodash');
const {nodes} = require('./NodeStore');
const {defaultUuid} = require('./Emulator');


const uuids = observable.map({});
const createDb = uuid => uuids.set(uuid, observable.map({}));
const retrieveDb = uuid => uuids.get(uuid);

const respondSuccess = (uuid, request_id, ws) => {
    if (ws) {
        ws.send(JSON.stringify(
            {
                msg: 'setup complete',
                response_to: request_id
            }
        ));
    } else {
        process.env.emulatorQuiet ||
            console.log(`******* SETUP: createDB ${uuid} *******`);
    }
};

const respondError = (uuid, request_id, ws) => {
    if (ws) {
        ws.send(JSON.stringify(
            {
                error: `Sorry, the uuid, ${uuid}, is already taken.`,
                response_to: request_id
            }
        ));
    } else {
        process.env.emulatorQuiet ||
            console.log(`******* SETUP: ${uuid} in already in uuids ********`);
    }
};

module.exports = {

    uuids,

    setup: ({'db-uuid':uuid, request_id}, ws) => {
        if (!uuids.has(uuid)) {
            createDb(uuid);

            respondSuccess(uuid, request_id, ws)

        } else {
            // respondError(uuid, request_id, ws) // respondSuccess for hackathon purposes
            respondSuccess(uuid, request_id, ws)
        }
    },

    read: ({'db-uuid':uuid, request_id, data:{key}}, ws) => {
        let data = retrieveDb(uuid);

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

    update: ({'db-uuid':uuid, request_id, data:{key, value}}, ws) => {

        let data = retrieveDb(uuid);

        data.set(key, value);

        ws.send(JSON.stringify(
            {
                response_to: request_id
            }
        ));
    },

    has: ({'db-uuid':uuid, request_id, data:{key}}, ws) => {

        let data = retrieveDb(uuid);

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

    'delete': ({'db-uuid':uuid, request_id, data:{key}}, ws) => {

        let data = retrieveDb(uuid);

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

    keys: ({'db-uuid': uuid, request_id}, ws) => {
        let data = retrieveDb(uuid);

        ws.send(JSON.stringify(
            {
                data: {
                    value: data.keys()
                },
                response_to: request_id
            }
        ));

    },

    getData: (uuid = defaultUuid) =>
        retrieveDb(uuid),

    setData: (uuid = defaultUuid, obj) => {
        let data = retrieveDb(uuid);
        data.clear();
        data.merge(obj);
    }
};

import {addCommandProcessor, receiveMessage} from "bluzelle-client-common/services/CommandService";
import {mapValues} from 'lodash';
import {removePreviousHistory, updateHistoryMessage} from './CommandQueueService';

const data = observable.map({});

export const getLocalDataStore = () => data;

export const touch = key =>
    data.has(key) ? data.get(key).set('mostRecentTimestamp', new Date().getTime())
                  : data.set(key, observable.map({ mostRecentTimestamp: new Date().getTime() }));

addCommandProcessor('aggregate', (data, ws) => {
    data.forEach( msg => {
        receiveMessage(msg, ws);
    });
});

addCommandProcessor('update', ({key, bytearray}) => {
    touch(key);

    if (bytearray) {
        data.get(key).set('bytearray', bytearray);
    }
});

addCommandProcessor('delete', ({key}) => {
    data.delete(key);

    removePreviousHistory();
    updateHistoryMessage(<span>Deleted keys <code key={1}>{JSON.stringify(key)}</code> from node.</span>);
});

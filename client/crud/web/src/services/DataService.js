import {addCommandProcessor} from "bluzelle-client-common/services/CommandService";
import {mapValues} from 'lodash';
import {removePreviousHistory, updateHistoryMessage} from './CommandQueueService';

const data = observable.map({});

export const getLocalDataStore = () => data;

export const touch = key =>
    data.has(key) ? data.get(key).set('mostRecentTimestamp', new Date().getTime())
                  : data.set(key, observable.map({ mostRecentTimestamp: new Date().getTime() }));

addCommandProcessor('keyListUpdate', keys => keys.forEach(touch));
addCommandProcessor('keyListDelete', keys => {
    keys.forEach(key => data.delete(key));

    removePreviousHistory();
    updateHistoryMessage(<span>Deleted keys <code key={1}>{JSON.stringify(keys)}</code> from node.</span>);

});

addCommandProcessor('bytearrayUpdate', ({key, bytearray}) => {
    touch(key);
    data.get(key).set('bytearray', new Uint8Array(bytearray));

    removePreviousHistory();
    updateHistoryMessage(<span>Updated <code key={1}>{key}</code> from node.</span>);

});
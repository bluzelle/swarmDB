import {getLocalDataStore} from "./DataService";
import {commandQueue, currentPosition, removePreviousHistory, updateHistoryMessage} from "./CommandQueueService";
import {sendToNodes} from "bluzelle-client-common/services/CommunicationService";
import {mapValues, extend, reduce} from 'lodash';

import {isObservableArray} from 'mobx';


const toSerializable = v =>
    v === 'deleted' ? v : toPlainArray(v);

const toPlainArray = typedArr => Array.from(typedArr);

const commandsToSave = () =>
    commandQueue.slice(0, currentPosition.get() + 1);

const addChangesFromCommand = (changes, command) =>
    extend(changes, command.onSave(changes));

const generateChanges = () => 
    reduce(commandsToSave(), addChangesFromCommand, {});

const buildRequests = changes => {
    const requests = [];

    Object.keys(changes).forEach(key => {
        if (changes[key] === 'deleted') {
            requests.push({
                cmd: 'destroy',
                data:
                    {
                        key: key,
                    }
            });
        } else {
            requests.push({
                cmd: 'update',
                data:
                    {
                        key: key,
                        bytearray: changes[key]
                    }
            });
        }
    });
    return requests;
};

const clearEditingData = () => {
    const data = getLocalDataStore();
    data.keys().forEach(key => data.has(key) && data.get(key).clear());
};

export const save = () => {

    const changes = generateChanges();

    const requests = buildRequests(changes);

    clearEditingData();

    sendToNodes('aggregate', requests);

    removePreviousHistory();
    updateHistoryMessage(<span>Saved.</span>);
};

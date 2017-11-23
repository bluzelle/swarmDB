import {addCommandProcessor} from 'services/CommunicationService'
import takeRight from 'lodash/takeRight'
import {updateNode} from "./NodeService";

const messages = observable.shallowArray([]);
global.messages = messages

addCommandProcessor('messages', action('messages', (arr, node) => arr.forEach(m => messages.push({...m, node, dstAddr: node.address}))));

(function() {
    let lastReceivedMessageIdx = 0;
    setInterval(() => {
        const newMessages = takeRight(messages, messages.length - lastReceivedMessageIdx);
        lastReceivedMessageIdx = messages.length;
        newMessages.reduce((result, message) => {
            result.add(message.node);
            return result;
        }, new Set())
            .forEach(node => updateNode({...node, lastMessageReceived: new Date().getTime()}));
    }, 700);
}());

export const getMessages = () => messages;
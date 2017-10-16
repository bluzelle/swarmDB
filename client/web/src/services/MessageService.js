import {addCommandProcessor} from 'services/CommunicationService'

const messages = observable.shallowArray([]);

addCommandProcessor('messages', action('messages', arr => arr.forEach(m => messages.push(m))));

export const getMessages = () => messages;
import {addCommandProcessor} from 'services/CommunicationService'

addCommandProcessor('setMaxNodes', action('setMaxNodes', num => settings.maxNodes = num));
addCommandProcessor('setMinNodes', action('setMinNodes', num => settings.minNodes = num));

export const settings = observable({
    maxNodes: 0,
    minNodes: 0
});

//autorun(() => socketState.get() === 'open' && untracked(getAllSettings));

const getAllSettings = () => {
    sendCommand('getMaxNodes', undefined);
    sendCommand('getMinNodes', undefined)
};



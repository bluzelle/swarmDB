export const commandProcessors = {};
let sentCommands = [];

export const socketState = {get: () => {}};

export const addCommandProcessor = (name, fn) => commandProcessors[name] = fn;

export const sendCommand = (cmd, data) => sentCommands.push({cmd: cmd, data: data});
export const receiveCommand = (cmd, data) => commandProcessors[cmd](data);

export const getSentCommands = () => sentCommands;
export const clearSentCommands = () => sentCommands = [];
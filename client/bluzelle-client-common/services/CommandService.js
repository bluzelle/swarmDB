const commandProcessors = [];

export const addCommandProcessor = (name, fn) => commandProcessors[name] = fn;

export const receiveMessage = (msg, node) => {
    commandProcessors[msg.cmd] ? commandProcessors[msg.cmd](msg.data, node) : console.error(`${msg.cmd} has no command processor`)
};

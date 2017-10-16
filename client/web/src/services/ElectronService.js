import {socketState, daemonUrl} from "./CommunicationService";

setTimeout(() => {
    global.electron || daemonUrl.set(window.location.host);

    if(global.electron) {
        setupIpc();
    }
});

const setupIpc = () => {
    autorun(() => send('setConnectionStatus', socketState.get()));
};

const send = (cmd, data) => electron.ipcRenderer.send(JSON.stringify({cmd: cmd, data: data}));


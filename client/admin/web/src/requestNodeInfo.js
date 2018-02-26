import {entryPointSocket} from "bluzelle-client-common/services/CommunicationService";

autorun(function requestNodeInfo() {
    const socket = entryPointSocket.get();

    if(socket) {
        socket.onopen = () =>
            socket.send(JSON.stringify({
                cmd: 'requestAllNodes'
            }));
    }
});
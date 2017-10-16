let socket;


autorun(() =>
    Session.get('ready') && startSocket()
);



const startSocket = () => {
    socket = new WebSocket(`ws://${window.location.host}/ws`);
    console.log(socket);
};

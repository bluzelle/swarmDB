const api = require('../api');

const WebSocketServer = require('websocket').server;
const http = require('http');
const reset = require('./reset');
const assert = require('assert');


// Run if testing in node, otherwise skip
(typeof window === 'undefined' ? describe.only : describe.skip)('redirect', () => {

	const port = 8105;

	let httpServer;


	beforeEach(reset);


	before(async () => {

		// Here we're going to mock the daemon with a simple redirect message.

		httpServer = http.createServer();
		await httpServer.listen(port);

		const ws = new WebSocketServer({
		    httpServer: httpServer,
		    autoAcceptConnections: true
		});


		ws.on('connect', connection => 
			connection.on('message', ({utf8Data: message}) => {

				const id = JSON.parse(message).request_id;

				connection.send(JSON.stringify({
					response_to: id,
					redirect: 'ws://localhost:8100', // Proper emulator
					data: JSON.parse(message)
				}));
			}));

	});


	after(() => httpServer.close());


	it('should follow a redirect and send the command to a different socket', async () => {

		await api.connect('ws://localhost:' + port, '71e2cd35-b606-41e6-bb08-f20de30df76c');
        await api.setup();

        await api.update('hey', 123);
        assert(await api.read('hey') === 123);

        await api.disconnect();

	});

});
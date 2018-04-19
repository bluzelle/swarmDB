const api = require('../api');

const WebSocketServer = require('websocket').server;
const http = require('http');


// Run if testing in node, otherwise skip
(typeof window === 'undefined' ? describe.only : describe.skip)('redirect', () => {

	const port1 = 8105;
	const port2 = 8106;

	let httpServer1;
	let httpServer2;

	before(async () => {

		httpServer1 = http.createServer();
		await httpServer1.listen(port1);

		const ws1 = new WebSocketServer({
		    httpServer: httpServer1,
		    autoAcceptConnections: true
		});


		ws1.on('connect', connection => 
			connection.on('message', ({utf8Data: message}) => {

				const id = JSON.parse(message).request_id;

				connection.send(JSON.stringify({
					response_to: id,
					redirect: 'ws://localhost:' + port2
				}));
			}));



		httpServer2 = http.createServer();
		await httpServer2.listen(port2);

		const ws2 = new WebSocketServer({
		    httpServer: httpServer2,
		    autoAcceptConnections: true
		});


		ws2.on('connect', connection => 
			connection.on('message', ({utf8Data: message}) => {

				const id = JSON.parse(message).request_id;

				connection.send(JSON.stringify({
					response_to: id,
					data: {
						value: "123"
					}
				}));
			}));

	});


	after(async () => {

		await httpServer1.close();
		await httpServer2.close();

	});


	it('should follow a redirect and send the command to a different socket', async () => {

		await api.connect('ws://localhost:' + port1, '71e2cd35-b606-41e6-bb08-f20de30df76c');
        api.setup();

        await api.read('hey');

	});

});
const http = require('http');
const WebSocketServer = require('websocket').server;
const reset = require('../emulator/Emulator').reset;



process.env.emulatorQuiet = true;


// Enable emulator resets between tests (see ../test/reset.js)

const httpServer = http.createServer();
httpServer.listen(8101, () => { console.log('Reset server listening!'); });

const ws = new WebSocketServer({
    httpServer: httpServer,
    autoAcceptConnections: true
});


ws.on('connect', connection => 
	connection.on('message', async () => {

		await reset();
		
		connection.send('');
	
	}));



// Compile the tests

const {exec} = require('child_process');

exec(`npx webpack --config ${__dirname}/webpage/webpack.config.js`, () => {

	console.log('Library & tests bundled!');
	console.log('Launching Chrome!');


	// Launch Chrome

	if(process.platform === 'linux') {

		exec(`google-chrome ${__dirname}/webpage/index.html`, () => {

			console.log('Chrome exited. Exiting process.');

			process.exit();

		});

	} else {

		// Mac

		exec(`open -a "Google Chrome" ${__dirname}/webpage/index.html`);

	}

});
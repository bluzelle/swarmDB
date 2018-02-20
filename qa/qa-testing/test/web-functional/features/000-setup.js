global.emulator = require('../../../emulator/Emulator');
const nodes = require('../../../emulator/NodeStore').nodes;

emulator.start(8200);

beforeEach('setup', () => {
    wrapAsync(() => Promise.all(emulator.shutdown()))();
    emulator.setMaxNodes(1);
    browser.waitUntil(() => nodes.keys().length === 1);
    browser.url('http://localhost:8200');
    browser.waitForExist('header');
});



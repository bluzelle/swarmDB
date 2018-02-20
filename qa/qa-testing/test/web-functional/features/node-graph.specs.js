const {clickTab} = require('../utils');

describe('Node graph tab', () => {

    beforeEach(() => clickTab('Node Graph'));

    describe('multiple nodes', () => {
        it('should show the number of nodes set by the emulator', () => {
            const NUM_OF_NODES = 3;

            emulator.setMaxNodes(NUM_OF_NODES);
            browser.waitUntil(() => browser.elements('circle').value.length === NUM_OF_NODES * 2);
        });
    });

    describe('individual nodes', () => {
        it('should display specs when mouseover on a node', () => {
            const node = emulator.getNodes()[0];
            browser.waitForExist(`g[data-test='node-${node.ip}-${node.port}']`);
            browser.moveToObject(`g[data-test='node-${node.ip}-${node.port}']`);
            browser.waitForExist(`td=${node.address}`);
            browser.waitForExist(`span*=${node.available}`);
            browser.waitForExist(`span*=${node.used}`);
            ['new', 'alive'].forEach( status => {
                browser.waitForExist(`div=${status}`);
            });
        });
    });
});

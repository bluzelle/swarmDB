const {clickTab} = require('../../utils');

describe('Node list load tests', () => {

    beforeEach(() => clickTab('Node List'));

    it('should be able to handle a lot of nodes quickly', () => {
        // Edge case: Each node extends the height of the parent div. If they cannot fit into browser window
        // (and be displayed) the test will fail.
        const NUM_OF_NODES = 10;

        emulator.setMaxNodes(NUM_OF_NODES);
        browser.waitUntil(() =>  browser.elements('div.react-grid-Canvas>div>div').value.length === NUM_OF_NODES, 15000);
    });
});

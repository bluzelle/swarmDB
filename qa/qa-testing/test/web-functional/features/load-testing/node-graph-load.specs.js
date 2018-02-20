const {clickTab} = require('../../utils');

describe('Node graph load tests', () => {

    beforeEach(() => clickTab('Node Graph'));

    it('should be able to handle a lot of nodes quickly', () => {
        const NUM_OF_NODES = 10;

        emulator.setMaxNodes(NUM_OF_NODES);
        browser.waitUntil(() =>  browser.elements('circle').value.length === NUM_OF_NODES * 2, 15000);
    });
});

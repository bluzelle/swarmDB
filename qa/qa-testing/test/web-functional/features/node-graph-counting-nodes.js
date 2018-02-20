const {clickTab} = require('../utils');

describe('Node graph tab', () => {

    beforeEach(() => clickTab('Node Graph'));

    describe('counting the nodes', () => {
        it('should show nodes equal to the number written on screen', () => {
            browser.waitForExist('div=1 Nodes');
            browser.waitUntil(() => browser.elements('circle').value.length === 2);
        });
    });
});

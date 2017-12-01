const {clickTab} = require('../utils');

describe('Node graph tab', () => {
    require('../getBaseElement')('body');

    beforeEach(() => clickTab('Node Graph'));

    describe('counting the nodes', () => {
        it('should show nodes equals to the number written on screen', () => {
        });
    });
});
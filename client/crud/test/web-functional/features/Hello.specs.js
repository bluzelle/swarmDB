describe('acceptance test for acceptance tests', () => {

    const body = require('../getBaseElement')('body');

    it('should contain a body', () => {
        expect(body()).to.not.equal(null);
    });

    it('body has a src attribute', () => {
        expect(body().getAttribute('src')).to.not.equal(null);
    });

});
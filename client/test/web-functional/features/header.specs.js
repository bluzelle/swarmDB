describe('web page header', () => {
    const header = require('../getBaseElement')('header');

    it('should exist', () => {
        expect(header().getAttribute('div>img', 'src')).to.contain('data');
    });
});
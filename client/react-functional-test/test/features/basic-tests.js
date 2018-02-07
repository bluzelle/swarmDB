const {start, close} = require('../testServer');
const {expect} = require('chai');

describe('Basic testing environment', () => {

    before(() => {
        start(8200);
        browser.url('http://localhost:8200');
    });

    after(() => {
        close();
    });


    it('should pass a test', () => {});

    it('should find an existing element', () => {
        browser.waitForExist('#react-root');
    });

    it('should fail for non-existent element', () => {
        expect(() => browser.waitForExist('#blah-blah')).to.throw();
    });

});
const request = require('request');

describe('http bluzelle manager interface',() => {
    it('should respond to the root', () => {
        request('http://localhost:8000', (error, response, body) => {
            expect(error).to.be.null;
            expect(response.statusCode).to.equal(200);
            expect(body).to.contain('Bluzelle Manager');
        })
    })
});
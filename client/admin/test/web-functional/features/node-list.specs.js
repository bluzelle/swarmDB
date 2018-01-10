describe('Node List tab', () => {

    describe('Table Headers', () => {
        const header = require('../getBaseElement')('div.react-grid-HeaderRow');

        beforeEach(() => {
            browser.waitForExist('=Node List', 2000);
            browser.click('=Node List');
        });
       it('should contain table headers', () => {
          ['Address', 'Status', 'Actions'].forEach(text => {
            header().waitForExist(`div.widget-HeaderCell__value*=${text}`);
          });
       });
    });
});
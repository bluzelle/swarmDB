describe('Basic WebDriverIO functionality', () => {

    it('has title Bluzelle', () => {

        browser.url('http://localhost:8200');
        expect(browser.getTitle()).to.equal('Bluzelle');

    });

    it('Can open two browser tabs.', () => {

        browser.url('http://localhost:8200');
        browser.newWindow('http://localhost:8200');

        const [firstWindow, secondWindow] = browser.getTabIds();


        // Tab IDs valid
        expect(firstWindow.length > 10).to.be.true;
        expect(secondWindow.length > 10).to.be.true;

        browser.close();

    });

});

describe('Settings tab', () => {
    require('../getBaseElement')('body');

    beforeEach(() => {
        browser.waitForExist('=Settings', 2000);
        browser.click('=Settings');
    });

    describe('Changing Max Nodes value', () => {
        it('should change the value of Max Nodes', () => {
            browser.click('button=Edit');
            browser.waitForExist('[type=number]', 2000);
            browser.setValue('[type=number]','6');
            browser.click('button=Set');
        });
    });
});
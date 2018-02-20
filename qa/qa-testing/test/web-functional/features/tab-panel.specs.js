describe('The tab panel', () => {

    it('should exist', () => {
        browser.waitForExist('a=Log');
        browser.waitForExist('a=Message List');
        browser.waitForExist('a=Node List');
        browser.waitForExist('a=Node Graph');
    });
});

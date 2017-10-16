module.exports = (selector) => {
    beforeEach(() => {
        browser.url('http://localhost:3002');
    });

    return () => {
        browser.waitForExist(selector, 2000);
        return browser.element(selector);
    };
};


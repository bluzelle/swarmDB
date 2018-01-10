module.exports = (selector) => {
    beforeEach(() => {
        browser.url('http://localhost:8200');
    });

    return () => {
        browser.waitForExist(selector, 2000);
        return browser.element(selector);
    };
};


module.exports = {
    clickTab: text => {
        browser.waitForExist(`=${text}`);
        browser.click(`=${text}`);
    }
};
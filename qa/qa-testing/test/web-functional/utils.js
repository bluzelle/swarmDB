module.exports = {
    clickTab: text => {
        browser.waitForExist(`=${text}`, 2000);
        browser.click(`=${text}`);
    }
};
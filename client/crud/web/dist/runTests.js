require('source-map-support').install({
    handleUncaughtExceptions: false
});

const { JSDOM } = require('jsdom');

const jsdom = new JSDOM('<!doctype html><html><body></body></html>');
const { window } = jsdom;



function copyProps(src, target) {
    const props = Object.getOwnPropertyNames(src)
        .filter(prop => typeof target[prop] === 'undefined')
        .map(prop => Object.getOwnPropertyDescriptor(src, prop));
    Object.defineProperties(target, props);
}
global.window = window;
global.document = window.document;
global.navigator = {
    userAgent: 'node.js',
    appVersion: 'node'
};

copyProps(window, global);

// START - crazy patch to get past a bug in react-vendor-prefix that is used by react-layout-pane
const oldGetComputedStyle = window.getComputedStyle
window.getComputedStyle = (...args) => {
    const x = oldGetComputedStyle.apply(window, args);
    x.OLink = '';
    return x;
};
// END - crazy patch


require('./generated/js/test.js');
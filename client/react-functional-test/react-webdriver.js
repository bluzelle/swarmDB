const TestUtils = require('react-dom/test-utils');
const ReactDOM = require('react-dom');


module.exports = {

    executeAsyncWithError: function(browser, f, ...args) {

        const a = browser.executeAsync(f, ...args);

        if (a.value !== null && typeof a.value === 'object' && a.value.stack) {
            const e = new Error();

            e.name = a.value.name;
            e.stack = a.value.stack;
            e.message = a.value.message;

            throw e;
        }

        return a;
    },


    findComponentsTest: function(componentName) {
        return browser.execute(
            componentName =>
                findComponentsBrowser(componentName).map(comp => comp.props),
            componentName
        ).value
    },


    // Testing exports finished.

    // Everything below this point is meant to be included in the application
    // itself.

    addHooks: function(hooks) {

        'root' in hooks || console.error('Must supply key "root" to addHooks');
        'components' in hooks || console.error('Must supply components to addHooks');


        hooks.TestUtils = TestUtils;
        hooks.ReactDOM = ReactDOM;

        window.hooks = hooks;


        // Usage: assert(() => 1 + 1 === 2);
        window.assert = fn => {
            if (typeof fn !== 'function') {
                throw new Error('Must pass function to assert.');
            }

            if (!fn()) {
                throw new Error(fn.toString());
            }
        };

        window.throwAsync = (err, done) => {
            done({
                name: err.name,
                stack: err.stack,
                message: err.message
            });
        };

        window.waitFor = (fn, done, timeout = 500) => {
            const start = new Date().getTime();

            const loop = (resolve, reject) => {
                try {
                    fn();
                    resolve();
                } catch (e) {
                    if (new Date().getTime() - start > timeout) {
                        e.name = "(waitFor timeout) " + e.name;
                        throwAsync(e, done);
                    }

                    setTimeout(loop, 100, resolve, reject);
                }
            };

            return new Promise(loop);
        };

        window.findComponentsBrowser = componentName => {

            if (!(componentName in window.hooks.components)) {
                throw new Error(`${componentName} is not a component.`);
            }

            return window.hooks.TestUtils.scryRenderedComponentsWithType(
                window.hooks.root,
                window.hooks.components[componentName]
            );
        };
    }
};
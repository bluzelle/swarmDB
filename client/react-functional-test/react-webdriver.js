const TestUtils = require('react-dom/test-utils');
const {Maybe} = require('simple-monads');


const findComponentsTest = function(componentName) {
    return browser.execute(
        componentName =>
            findComponentsBrowser(componentName)
                .map(comp => ({ props: comp.props })),
        componentName
    ).value
};


module.exports = {

    findComponentsTest: findComponentsTest,

    findComponentTest: function(componentName) {
        return findComponentsTest(componentName)[0];
    },

    addHooks: function(root, components) {

        window.findComponentsBrowser = componentName =>
            Maybe.of(components[componentName])
                .orElse(() => { throw new Error('Component does not exist'); })
                .flatMap(component =>
                    TestUtils.scryRenderedComponentsWithType(
                        root,
                        component
                    )
                );
    }
};
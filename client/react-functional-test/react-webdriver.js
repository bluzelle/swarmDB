const TestUtils = require('react-dom/test-utils');
const {Maybe} = require('simple-monads');
const {pickBy} = require('lodash');




const findComponentsTest = function(componentName) {
    return browser.execute(
        componentName =>
            findComponentsBrowser(componentName)
                .map(restrictComponent),
        componentName
    ).value
};


const isNonTrivial = value =>
    typeof value === 'number' || typeof value === 'string';

const filterNonTrivialProps = props =>
    pickBy(props, isNonTrivial);

const restrictComponent = comp => ({
    props: filterNonTrivialProps(comp.props)
});


module.exports = {

    findComponentsTest: findComponentsTest,

    findComponentTest: function(componentName) {
        return findComponentsTest(componentName)[0];
    },

    addHooks: function(root, components) {

        window.findComponentsBrowser = componentName =>
            Maybe.of(components[componentName])
                .orElse(() => { throw new Error(`Component "${componentName}" does not exist`); })
                .flatMap(component =>
                    TestUtils.scryRenderedComponentsWithType(
                        root,
                        component
                    )
                );


        window.restrictComponent = restrictComponent;

    }
};
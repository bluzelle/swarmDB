import {start, close} from '../testServer';
import {expect, assert} from 'chai';

import {findComponentsTest} from "../../react-webdriver";


describe('React webdriver', () => {

    before(() => {
        start(8200);
    });

    beforeEach(() => {
        browser.url('http://localhost:8200');
    });

    after(() => {
        close();
    });



    it('should find components', () => {

        // Here we are calling findComponents in the chimp context:

        const comps = findComponentsTest('SimpleComponent');

        assert(comps.length === 1);
        assert(comps[0].text === 'hello world');


        // And here in the browser context:
        // (Must access from window to avoid scoping errors in our test runner)

        browser.execute(() => {

            const comp = findComponentsBrowser('SimpleComponent');
            window.assert(() => comp[0].props.text === 'hello world');

        });


        // findComponentsBrowser() in the browser context returns an array of
        // React components whereas findComponentsTest() returns
        // an array of property objects, since other data is not serializable.

    });

    it('should throw an error for non-existent components', () => {

        expect(() => findComponentsTest('agieogjeroi')).to.throw();

    });

});
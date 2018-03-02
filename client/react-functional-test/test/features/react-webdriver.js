import {start, close} from '../testServer';
import {expect} from 'chai';

import {findComponentTest, findComponentsTest} from "../../react-webdriver";


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

        const comp = findComponentTest('SimpleComponent');
        expect(comp.props.text).to.equal('hello world');


        const props = browser.execute(() =>
                findComponentsBrowser('SimpleComponent')[0].props).value;

        expect(props.text).to.equal('hello world');

    });

    it('should throw an error for non-existent components', () => {

        expect(() => findComponentsTest('agieogjeroi')).to.throw();

    });


    // Returning arbitrary data through browser.execute() will result in a
    // stack overflow error.

    // We currently restrict props to "trivial" types: numbers and strings.
    it('should filter non-trivial props', () => {

        const comp = findComponentTest('SimpleComponent');
        expect(comp.props.function).to.be.undefined;

    });

});
import {start, setData} from '../emulator/Emulator';
import {reset} from "../util";
import {findComponentsTest} from "react-functional-test";

describe('Emulator connections', () => {

    before(() => start());
    beforeEach(() => reset());

    it('should be able to connect to the emulator', () => {

        browser.url('http://localhost:8200/?test');

        browser.waitForExist('button=Go');
        browser.element('button=Go').click();


        // TODO: Sack `findComponentsTest`

        browser.waitUntil(() => findComponentsTest('KeyList').length > 0)

    });

});

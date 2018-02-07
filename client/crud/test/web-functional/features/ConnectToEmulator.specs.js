import {start, shutdown, setData} from '../emulator/Emulator';
import {findComponentsTest} from "react-functional-test";

describe('Emulator connections', () => {

    it('should be able to connect to the emulator', () => {

        start();

        browser.url('http://localhost:8200/?test');

        browser.waitForExist('button=Go');

        browser.element('button=Go').click();


        // TODO: rebase as single commit on devel


        // 1. New branch
        // 2. Refactoring
        // 3. Squash into one commit <--- git rebase -i

        browser.waitUntil(() => findComponentsTest('KeyList').length > 0)

        shutdown();

    });


    it.only('should be able to set data', () => {

        const data = {
            text: [1, 72, 101, 108, 108, 111, 32, 119, 111, 114, 108, 100, 44, 32, 116, 104, 105, 115, 32, 105, 115, 32, 115, 111, 109, 101, 32, 112, 108, 97, 105, 110, 32, 116, 101, 120, 116, 46]
        };

        start();
        setData(data);


        browser.url('http://localhost:8200/?test');

        browser.waitForExist('button=Go');
        browser.element('button=Go').click();


        // Max. call stack size exceeded?
        browser.waitUntil(() =>
            findComponentsTest('KeyListItem').some(props => props.keyname === 'text'), 10000);

        shutdown();

    });

});
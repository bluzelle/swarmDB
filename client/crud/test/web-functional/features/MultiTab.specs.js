import {start, setData, getData} from "../emulator/Emulator";
import {reset, checkUndo} from "../util";
import {findComponentsTest} from "react-functional-test";
import {hasKey, hasNoKey, newField, remove, save, selectKey, setJSON, refresh} from "../pageActions";

describe('Multi-client functionality.', () => {

    let firstWindow = null,
        secondWindow = null;


    before(() => {

        start();

        browser.url('http://localhoast:8200/?test');
        browser.newWindow('http://localhost:8200/?test');

        browser.waitUntil(() => browser.getTabIds().length == 2);

        const tabs = browser.getTabIds();
        firstWindow = tabs[0];
        secondWindow = tabs[1];

    });

    beforeEach(() => {

        reset();

        browser.switchTab(firstWindow);
        browser.url('http://localhost:8200/?test');

        browser.waitForExist('button=Go');
        browser.element('button=Go').click();


        browser.switchTab(secondWindow);
        browser.url('http://localhost:8200/?test');


        browser.waitForExist('button=Go');
        browser.element('button=Go').click();


        browser.switchTab(firstWindow);

    });


    it('should be able to connect both clients', () => {});


    const saveKey = () => {

        browser.switchTab(firstWindow);

        newField('hello', 'JSON');

        save();

        browser.switchTab(secondWindow);

        refresh();

        browser.waitForExist('button*=hello');

    };

    it('should be able to save a key on one client and load it on another', () => {

       saveKey();

    });


    it('should be able to delete a key on one client and load it on another', () => {

        saveKey();

        browser.switchTab(firstWindow);

        selectKey('hello'); // This actually deselects the key
        remove('hello'); // This selects it and then clicks remove

        browser.switchTab(secondWindow);

        refresh();

        browser.waitForExist('button*=hello', 500, true);

    });

});

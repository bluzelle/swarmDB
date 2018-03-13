import {start, setData} from "../emulator/Emulator";
import {reset, checkUndo} from "../util";
import {findComponentsTest} from "react-functional-test";
import {hasKey, hasNoKey, newField, remove, save, selectKey, setJSON, refresh} from "../pageActions";

describe('Multi-client functionality.', () => {

    let firstWindow = null,
        secondWindow = null;


    before(() => {

        start();

        browser.url('http://localhost:8200/?test');
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

        browser.waitUntil(() => findComponentsTest('KeyList').length, 2000);


        browser.switchTab(secondWindow);
        browser.url('http://localhost:8200/?test');


        browser.waitForExist('button=Go');
        browser.element('button=Go').click();

        browser.waitUntil(() => findComponentsTest('KeyList').length, 2000);

        browser.switchTab(firstWindow);

    });


    it('should be able to connect both clients', () => {});

    it('update keys from emulator', () => {

        setData({
            myKey: [0, 1, 2, 3, 4]
        });


        hasKey('myKey');

        browser.switchTab(secondWindow);

        hasKey('myKey');

        setData({});

        browser.switchTab(firstWindow);

        hasNoKey('myKey');

        browser.switchTab(secondWindow);

        hasNoKey('myKey');


        setData({
            helloworld: [9, 8, 7, 6, 5]
        });

        browser.switchTab(firstWindow);

        hasKey('helloworld');

        browser.switchTab(secondWindow);

        hasKey('helloworld');

    });


    it('should send new keys from one client to the other', () => {

        newField('newfield', 'JSON Data');
        save();

        browser.switchTab();

        hasKey('newfield');

    });


    it('should communicate deletion of keys from one client to the other', () => {

        setData({
            removeMe: [9, 2, 3]
        });


        remove('removeMe');

        save();

        browser.switchTab();

        hasNoKey('removeMe');

    });


    it('should communicate complex changes', () => {

        setData({
            hello: [9, 1, 2],
            world: [8, 8, 8],
            aKey: [9, 5, 3]
        });

        remove('hello');

        newField('newfield', 'JSON Data');
        newField('sometext', 'Plain Text');

        save();

        browser.switchTab();

        hasNoKey('hello');
        hasKey('newfield');
        hasKey('sometext');

    });


    // This fails but it shouldn't

    // it.only('should have a refresh button for object type', () => {
    //
    //     newField('json', 'JSON Data');
    //
    //     setJSON('{}');
    //
    //     save();
    //
    //     browser.switchTab(secondWindow);
    //
    //     hasKey('json');
    //
    //     selectKey('json');
    //
    //     setJSON('[ 1, 2, "crazy text"]');
    //
    //     save();
    //
    //     browser.switchTab(firstWindow);
    //
    //     refresh('json');
    //
    //     browser.waitForExist('span*=crazy text');
    //
    // });
    //

    it('should have a refresh button for text type', () => {

        newField('text', 'Plain Text');

        save();

        browser.switchTab(secondWindow);

        hasKey('text');

        selectKey('text');

        browser.waitForExist('textarea');
        browser.element('textarea').click();

        browser.keys(['Testing text']);

        save();

        browser.switchTab(firstWindow);

        refresh('text');

        browser.waitForExist('textarea*=Testing text');

    });


    // Doesn't overwrite old file on save.


    // it('should have a refresh button for file type', () => {
    //
    //     browser.waitForExist('.glyphicon-plus');
    //
    //     newField('file', 'File');
    //
    //     save();
    //
    //     browser.switchTab(secondWindow);
    //
    //     selectKey('file');
    //
    //     browser.switchTab(firstWindow);
    //
    //
    //     const path = __dirname + '/testfile.txt';
    //
    //     browser.chooseFile('input[type=file]', path);
    //
    //     browser.element('input[type=submit]').click();
    //
    //     save();
    //
    //     browser.switchTab(secondWindow);
    //
    //     browser.waitForExist('.glyphicon-refresh');
    //
    //     browser.click('.glyphicon-refresh');
    //
    //     browser.waitForExist('div*=0 bytes', 500, true);
    //
    // });

});
import {start, setData, getData} from "../emulator/Emulator";
import {reset, checkUndo} from "../util";
import {findComponentsTest} from "react-functional-test";
import {hasKey, hasNoKey, newField, remove, save, selectKey, setJSON, refresh} from "../pageActions";

describe('KeyList functionality.', () => {

    before(() => start());

    beforeEach(() => {

        reset();
        setData({});

        browser.url('http://localhost:8200/?test');

        browser.waitForExist('button=Go');
        browser.element('button=Go').click();

    });


    it('should undo new fields', () => {

        newField('mytext', 'JSON');

        checkUndo({
            verifyDo: () =>
                browser.waitForExist('span*=mytext'),
            verifyUndo: () =>
                browser.waitForExist('span*=mytext', 500, true)
        });

    });


    it('should undo field deletion', () => {

        newField('mytext', 'JSON');

        browser.click('button*=mytext');

        remove('mytext');

        checkUndo({
            verifyDo: () =>
                browser.waitForExist('span*=mytext', 500, true),
            verifyUndo: () =>
                browser.waitForExist('span*=mytext')
        });

    });

});

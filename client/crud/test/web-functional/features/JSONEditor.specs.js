import {start, setData} from "../emulator/Emulator";
import {reset, checkUndo} from "../util";
import {newField, setJSON} from "../pageActions";

describe('JSON Editor functionality.', () => {

    before(() => start());

    beforeEach(() => {

        reset();
        setData({});

        browser.url('http://localhost:8200/?test');

        browser.waitForExist('button=Go');
        browser.element('button=Go').click();

        browser.waitForExist('.glyphicon-plus');

        newField('json', 'JSON Data');

    });



    const editJSON = () => {

        setJSON('[1, "crazy text", 3, 4]');

        browser.waitForExist('span*=crazy text');


        // Since we use nested spans in the styling, we want to click
        // only the innermost one (i.e. the last one) to activate the
        // input.
        const spans = browser.elements('span*=crazy text').value;
        const last = spans[spans.length - 1];

        last.click();

        browser.waitForExist('input');

        browser.setValue('input', '{ "mykey": 123 }');
        browser.submitForm('input');


        browser.waitForExist('span*=mykey');

    };

    it('should be able to edit json directly', () => {

        editJSON();

    });

    it('should be able to edit json directly w/ undo', () => {

        editJSON();

        checkUndo({
            verifyDo: () =>
                browser.waitForExist('span*=mykey'),
            verifyUndo: () =>
                browser.waitForExist('span*=crazy text')
        });

    });


    const objPlus = () => {

        browser.moveToObject('span*=(0 entries)');

        const plus = browser.elements('.glyphicon-plus').value[1];

        plus.click();

        browser.keys(['myKey', 'Enter', '132', 'Enter']);

    };


    it('should have a working plus button for objects', () => {

        objPlus();

    });

    it('should have a working plus button for objects w/ undo', () => {

        objPlus();

        checkUndo({
            verifyDo: () => {
                browser.waitForExist('span*=myKey');
                browser.waitForExist('span*=132');
            },
            verifyUndo: () => {
                browser.waitForExist('span*=myKey', 500, true);
                browser.waitForExist('span*=132', 500, true);
            }
        });

    });



    const objDel = () => {

        browser.moveToObject('span*=(0 entries)');

        browser.click('.glyphicon-pencil');

        browser.waitForExist('input');

        browser.keys(['{ "keyA": 123 }', 'Enter']);

        browser.moveToObject('span*=123');

        browser.waitForExist('.glyphicon-remove');

        browser.elements('.glyphicon-remove').value[1].click();

        browser.waitForExist('span*=123', 500, true);

    };

    it('should have a working delete button for objects', () => {

        objDel();

    });


    it('should have a working delete button for objects w/ undo', () => {

        objDel();

        checkUndo({
            verifyDo: () =>
                browser.waitForExist('span*=123', 500, true),
            verifyUndo: () =>
                browser.waitForExist('span*=123')
        });

    });



    const plusArray = () => {

        browser.moveToObject('span*=(0 entries)');

        browser.click('.glyphicon-pencil');

        browser.keys(['[]', 'Enter']);

        browser.elements('.glyphicon-plus').value[1].click();

        browser.keys(['999', 'Enter']);

    };

    it('should have a working plus button for arrays', () => {

        plusArray();

    });


    it('should have a working plus button for arrays w/undo', () => {

       plusArray();

        checkUndo({
            verifyDo: () =>
                browser.waitForExist('span*=999'),
            verifyUndo: () =>
                browser.waitForExist('span*=999', 500, true)
        });

    });



    const arrDel = () => {

        browser.moveToObject('span*=(0 entries)');

        browser.click('.glyphicon-pencil');

        browser.keys(['[654]', 'Enter']);

        browser.moveToObject('span*=654');

        browser.elements('.glyphicon-remove').value[1].click();

    };

    it('should have a working delete button for arrays', () => {

        arrDel();

    });

    it('should have a working delete button for arrays w/ undo', () => {

        arrDel();

        checkUndo({
            verifyDo: () =>
                browser.waitForExist('span*=654', 500, true),
            verifyUndo: () =>
                browser.waitForExist('span*=654')
        });

    });

});
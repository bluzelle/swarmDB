import {start, setData} from "../emulator/Emulator";
import {reset, checkUndo} from "../util";
import {findComponentsTest} from "react-functional-test";
import {newField, setJSON} from "../pageActions";

describe('KeyList functionality', () => {

    before(() => start());

    beforeEach(() => {

        const data = {
            textA: [1, 72, 101, 108, 108, 111, 32, 119, 111, 114, 108, 100, 44, 32, 116, 104, 105, 115, 32, 105, 115, 32, 115, 111, 109, 101, 32, 112, 108, 97, 105, 110, 32, 116, 101, 120, 116, 46],
            textB: [1, 72, 99, 108, 108, 111, 32, 119, 111, 114, 108, 100, 44, 32, 116, 104, 105, 115, 32, 105, 115, 32, 115, 111, 109, 101, 32, 112, 108, 97, 105, 110, 32, 116, 101, 120, 116, 46],
            objC: [0, 1, 2, 3, 4, 111, 114, 108, 100, 44, 32, 116, 104, 105, 115, 32, 105, 115, 32, 115, 111, 109, 101, 32, 112, 108, 97, 105, 110, 32, 116, 101, 120, 116, 46]
        };

        reset();
        setData(data);

        browser.url('http://localhost:8200/?test');

        browser.waitForExist('button=Go');
        browser.element('button=Go').click();

        browser.waitUntil(() => findComponentsTest('KeyList').length, 2000);

    });

    it('should render the keys in the database on start', () => {

        browser.waitUntil(() => findComponentsTest('KeyListItem').length, 2000);

        const comps = findComponentsTest('KeyListItem');

        expect(comps.some(comp => comp.props.keyname === 'textA')).to.be.true;
        expect(comps.some(comp => comp.props.keyname === 'textB')).to.be.true;
        expect(comps.some(comp => comp.props.keyname === 'objC')).to.be.true;
        expect(comps.some(comp => comp.props.keyname === 'blah')).to.be.false;

    });

    it('should render keys in alphabetical order', () => {

        browser.waitUntil(() => findComponentsTest('KeyListItem').length, 2000);

        const comps = findComponentsTest('KeyListItem');

        const textAInd = comps.findIndex(comp => comp.props.keyname === 'textA');
        const textBInd = comps.findIndex(comp => comp.props.keyname === 'textB');
        const objCInd = comps.findIndex(comp => comp.props.keyname === 'objC');

        expect(textAInd < textBInd).to.be.true;
        expect(objCInd < textAInd).to.be.true;

    });

    it('should be able to select a key', () => {

        browser.waitForExist('button*=textA');

        // First click downloads the data and clears history
        browser.element('button*=textA').click();

        // Unselect and reselect
        browser.element('button*=textA').click();
        browser.element('button*=textA').click();


        // Verify that selection can be undone
        checkUndo({
            verifyDo: () =>
                browser.waitForExist('textarea'),
            verifyUndo: () =>
                browser.waitForExist('textarea', 500, true)
        });

    });


    describe('adding keys', () => {

        it('should be able to add a key with object type', () => {

            browser.waitForExist('.glyphicon-plus');

            newField('test', 'JSON Data');

            checkUndo({
                verifyDo: () =>
                    browser.waitForExist('button*=test'),
                verifyUndo: () =>
                    browser.waitForExist('button*=test', 500, true)
            });

        });

        it('should be able to add a key with text type', () => {

            browser.waitForExist('.glyphicon-plus');

            newField('test', 'Plain Text');

            checkUndo({
                verifyDo: () =>
                    browser.waitForExist('button*=test'),
                verifyUndo: () =>
                    browser.waitForExist('button*=test', 500, true)
            });

        });

    });


    it('should not display icons on undownloaded keys', () => {

        browser.waitForExist('span*=textA');

        expect(browser.isExisting('.glyphicon-font')).to.be.false;

    });

    it('should display icons on downloaded keys', () => {

        browser.waitForExist('button*=textA');
        browser.element('button*=textA').click();

        expect(browser.isExisting('.glyphicon-font')).to.be.true;

    });

    it('should be able to rename a key after downloading', () => {

        browser.waitForExist('button*=textA');
        browser.element('button*=textA').click();

        browser.element('span=textA').click();

        browser.keys(['newkey', 'Enter']);

        checkUndo({
            verifyDo: () =>
                browser.waitForExist('span=newkey'),
            verifyUndo: () =>
                browser.waitForExist('span=textA')
        });

    });

    it('should be able to delete a key', () => {

        browser.waitForExist('button*=textA');
        browser.element('button*=textA').click();

        browser.element('.glyphicon-remove').click();

        checkUndo({
            verifyDo: () =>
                expect(browser.isExisting('button*=textA')).to.be.false,
            verifyUndo: () =>
                expect(browser.isExisting('button*=textA')).to.be.true
        });

    });


    // Also fails under existing implementation.

    // it('should not be able to rename a key to another pre-existing keyname', () => {
    //
    //     browser.waitForExist('span*=textA');
    //     browser.element('span=textA').click();
    //
    //     const rename = () =>
    //         browser.keys(['textB', 'Enter']);
    //
    //     expect(rename).to.throw();
    //
    // });


    // Weird replication of old data for some reason.

    // it('should be able to add and edit several new json keys', () => {
    //
    //     newField('json1', 'JSON Data');
    //
    //     setJSON('[1, 2, 3]');
    //
    //     newField('json2', 'JSON Data');
    //
    //     setJSON('{ "hello": "world" }');
    //
    // });

});
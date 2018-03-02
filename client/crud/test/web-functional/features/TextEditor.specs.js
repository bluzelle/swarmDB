import {start, setData} from "../emulator/Emulator";
import {reset, checkUndo} from "../util";

describe('Text Editor functionality.', () => {

    before(() => start());

    beforeEach(() => {

        const data = {
            text: [1, 72, 101, 108, 108, 111]
        };

        reset();
        setData(data);

        browser.url('http://localhost:8200/?test');

        browser.waitForExist('button=Go');
        browser.element('button=Go').click();

        browser.waitForExist('button*=text', 1000);

    });

    it('should be able to type into the text area and have it persist', () => {

        browser.element('button*=text').click();

        browser.waitForExist('textarea');
        browser.element('textarea').click();

        browser.keys(['My additional text here']);

        // Defocus
        browser.element('img').click();

        // Reselect
        browser.element('button*=text').click();
        browser.element('button*=text').click();


        checkUndo({
            verifyDo: () => {
                browser.waitForExist('textarea*=My additional text here');
            },
            verifyUndo: () => {
                browser.waitForExist('textarea*=My additional text here', 500, true);
            }
        });

    });

});
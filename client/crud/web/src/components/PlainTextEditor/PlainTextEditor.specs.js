import {observableMapRecursive as omr} from "../../util/mobXUtils";
import {PlainTextEditor, textToKeyData, interpret} from "./PlainTextEditor";
import {SimpleExecute, UndoRedoTest} from "../../services/CommandQueueService.specs";
import PropTypes from 'prop-types';

describe('Plain text editor', () => {
    it('should have a textToKeyData and an interpret function that cancel each other out', () => {
        expect(interpret(omr(textToKeyData("testing")))).to.equal("testing");
    });

    it('should respond to changes in the text area', () => {

        const keyData = omr(textToKeyData("hello world"));

        const wrapper = mount(
            <PlainTextEditor
                keyData={keyData}
                keyName='myText'/>, SimpleExecute);


        // Editing "local" state (unsaved)
        wrapper.find('textarea')
            .simulate('change', { target: { value: "Different text" }});

        expect(keyData.get('localChanges')).to.equal("hello world");

        // Verifying real state (i.e. push to command queue)
        wrapper.find('textarea')
            .simulate('blur');

        expect(keyData.get('localChanges')).to.equal("Different text");

    });


    it('should have correct undo/redo functionality', () => {

        const keyData = omr(textToKeyData("hello world"));

        const context = UndoRedoTest({
            verifyBefore: () => {
                expect(keyData.get('localChanges')).to.equal("hello world");
            },
            verifyAfter: () => {
                expect(keyData.get('localChanges')).to.equal("new string");
            }
        });

        const wrapper = mount(
            <PlainTextEditor
                keyData={keyData}
                keyName='myText'/>, context);

        wrapper.find('textarea')
            .simulate('change', { target: { value: "new string" }})
            .simulate('blur');

    });

});
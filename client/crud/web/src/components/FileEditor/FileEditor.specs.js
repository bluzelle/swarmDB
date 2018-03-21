import {FileEditor, defaultKeyData} from "./FileEditor";
import {SimpleExecute, UndoRedoTest} from "../../services/CommandQueueService.specs";

describe('File editor', () => {

    it('should render the file name', () => {

        const keyData = defaultKeyData;

        keyData.set('filename', 'hello.jpg');

        const wrapper = mount(
            <FileEditor keyData={keyData} keyName={''}/>, SimpleExecute);

        expect(wrapper.someWhere(el => el.text().includes('hello.jpg'))).to.be.true;
        expect(wrapper.everyWhere(el => el.text().includes('false.png'))).to.be.false;

    });


    // Upload behavior cannot be tested with Enzyme.

    // See FileEditor.specs.js in functional tests.

});
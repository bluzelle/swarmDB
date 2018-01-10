import {KeyList} from "./KeyList";
import {toJS} from 'mobx';
import {observableMapRecursive as omr} from "../../util/mobXUtils";
import {textToKeyData} from "../PlainTextEditor";
import {SimpleExecute, UndoRedoTest} from "../../services/CommandQueueService.specs";

import {selectedKey} from "./KeyList";

describe('KeyList', () => {

    beforeEach(() => {
        selectedKey.set(null);
    });

    const findLabel = (wrapper, word) =>
        wrapper.find(BS.ListGroupItem)
            .filterWhere(el => el.text().includes(word));



    it('should be able to select a new key', () => {

        const obj = omr({
            hello: textToKeyData("world"),
            goodbye: textToKeyData("cruel world")
        });

        const context = UndoRedoTest({
            verifyBefore: () => {
                expect(selectedKey.get()).to.equal(null);
            },
            verifyAfter: () => {
                expect(selectedKey.get()).to.equal('hello');
            }
        });


        const wrapper = mount(<KeyList obj={obj}/>, context);

        findLabel(wrapper, 'hello').simulate('click');
    });


    it('should be able to delete a key', () => {

        const obj = omr({ a: textToKeyData("2") });
        selectedKey.set('a');

        const context = UndoRedoTest({
            verifyBefore: () => {
                expect(obj.has('a'));
            },
            verifyAfter: () => {
                expect(toJS(obj)).to.deep.equal({});
            }
        });

        const wrapper = mount(<KeyList obj={obj}/>, context);

        wrapper.find(BS.Button)
            .at(1)
            .simulate('click');

    });


    it('should be able to add a key', () => {

        const obj = omr({});

        const context = UndoRedoTest({
            verifyBefore: () => {
                expect(toJS(obj)).to.deep.equal({});
            },
            verifyAfter: () => {
                expect(obj.has('some key here'));
            }
        });

        const wrapper = mount(<KeyList obj={obj}/>, context);

        wrapper.find(BS.Button)
            .at(0)
            .simulate('click');

        wrapper.find('input')
            .simulate('change', {target: {value: 'some key here'}})
            .simulate('submit');

        wrapper.find(BS.ListGroupItem)
            .filterWhere(el => el.text().includes('JSON'))
            .simulate('click');

    });



    it('should not add a key with a blank field', () => {

        const obj = omr({});

        const wrapper = mount(<KeyList obj={obj}/>, SimpleExecute);

        wrapper.find(BS.Button)
            .at(0)
            .simulate('click');

        wrapper.find('form')
            .simulate('submit');

        expect(obj.toJS()).to.deep.equal({});

    });

});
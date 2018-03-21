import {defaultKeyData as jsonDefault} from "../../JSONEditor";
import {defaultKeyData as textDefault} from "../../PlainTextEditor";
import {defaultKeyData as fileDefault} from "../../FileEditor/FileEditor";

import {ObjIcon} from "../../ObjIcon";
import {enableExecution} from "../../../services/CommandQueueService";
import {selectedKey} from "../KeyList";


@enableExecution
export class TypeModal extends Component {

    chooseJSON() {
        this.addNewKey(jsonDefault(), 'JSON');
    }

    chooseText() {
        this.addNewKey(textDefault(), 'plain text');
    }

    chooseFile() {
        this.addNewKey(fileDefault(), 'file');
    }

    addNewKey(keyData, typeName) {
        const {obj, keyField} = this.props;
        const oldSelection = selectedKey.get();
        const serial = keyData.get('bytearray').slice();

        this.context.execute({
            doIt: () => {
                obj.set(keyField, keyData);
                selectedKey.set(keyField);
            },
            undoIt: () => {
                selectedKey.set(oldSelection);
                obj.delete(keyField)
            },
            onSave: () => ({
                [keyField]: serial
            }),
            message: <span>Created <code key={1}>{keyField}</code> as {typeName}.</span>
        });

        this.props.onHide();
    }


    render() {
        return (
            <BS.Modal show={true} onHide={this.props.onHide}>
                <BS.Modal.Header closeButton>
                    <BS.Modal.Title>
                        Select Key Type
                    </BS.Modal.Title>
                </BS.Modal.Header>
                <BS.Modal.Body>
                    <BS.ListGroup>
                        <BS.ListGroupItem onClick={this.chooseJSON.bind(this)}>
                            <ObjIcon keyData={jsonDefault()}/>
                            JSON Data
                        </BS.ListGroupItem>
                        <BS.ListGroupItem onClick={this.chooseText.bind(this)}>
                            <ObjIcon keyData={textDefault()}/>
                            Plain Text
                        </BS.ListGroupItem>
                        <BS.ListGroupItem onClick={this.chooseFile.bind(this)}>
                            <ObjIcon keyData={fileDefault()}/>
                            File
                        </BS.ListGroupItem>
                    </BS.ListGroup>
                </BS.Modal.Body>
            </BS.Modal>
        );
    }
}
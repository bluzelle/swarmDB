import {defaultKeyData as jsonDefault} from "../JSONEditor";
import {defaultKeyData as textDefault} from "../PlainTextEditor";
import {ObjIcon} from "../ObjIcon";
import {executeContext} from "../../services/CommandQueueService";
import {selectedKey} from "./KeyList";

@executeContext
export class NewKeyTypeModal extends Component {

    chooseJSON() {

        // TODO: have observable.map by default on these defaults
        this.addNewKey(observable.map(jsonDefault), 'JSON');
    }

    chooseText() {
        this.addNewKey(observable.map(textDefault), 'plain text');
    }

    addNewKey(keyData, typeName) {
        const {obj, keyField} = this.props;
        const oldSelection = selectedKey.get();

        this.context.execute({
            doIt: () => {
                obj.set(keyField, keyData);
                selectedKey.set(keyField);
            },
            undoIt: () => {
                selectedKey.set(oldSelection);
                obj.delete(keyField)
            },
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
                            <ObjIcon keyData={observable.map(jsonDefault)}/>
                            JSON Data
                        </BS.ListGroupItem>
                        <BS.ListGroupItem onClick={this.chooseText.bind(this)}>
                            <ObjIcon keyData={observable.map(textDefault)}/>
                            Plain Text
                        </BS.ListGroupItem>
                    </BS.ListGroup>
                </BS.Modal.Body>
            </BS.Modal>
        );
    }
}
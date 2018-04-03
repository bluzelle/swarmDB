import {JSONIcon, TextIcon, FileIcon} from "../../ObjIcon";
import {selectedKey, refreshKeys} from "../KeyList";
import {update, remove} from 'bluzelle';
import {execute} from '../../../services/CommandQueueService';


export class TypeModal extends Component {

    chooseJSON() {
        this.addNewKey({}, 'JSON');
    }

    chooseText() {
        this.addNewKey('', 'plain text');
    }

    chooseFile() {
        this.addNewKey(new ArrayBuffer(), 'file');
    }


    addNewKey(keyData, typeName) {

        const oldSelection = selectedKey.get();


        execute({
            doIt: () => new Promise(resolve =>
                update(this.props.keyField, keyData).then(() =>
                    refreshKeys().then(resolve))),

            undoIt: () => new Promise(resolve =>
                remove(this.props.keyField).then(() =>
                    refreshKeys().then(resolve))),

            message: <span>Added field <code key={1}>{this.props.keyField}</code>.</span>
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
                            <JSONIcon/>
                            <span style={{marginLeft: 15}}>JSON Data</span>
                        </BS.ListGroupItem>
                        <BS.ListGroupItem onClick={this.chooseText.bind(this)}>
                            <TextIcon/>
                            <span style={{marginLeft: 15}}>Plain Text</span>
                        </BS.ListGroupItem>
                        <BS.ListGroupItem onClick={this.chooseFile.bind(this)}>
                            <FileIcon/>
                            <span style={{marginLeft: 15}}>File</span>
                        </BS.ListGroupItem>
                    </BS.ListGroup>
                </BS.Modal.Body>
            </BS.Modal>
        );
    }
}
import {enableExecution} from "../../services/CommandQueueService";
import {FileName} from './FileName';
import {FileSize} from "./FileSize";

export const PREFIX = 2;

@observer
@enableExecution
export class FileEditor extends Component {

    onSubmit(e) {

        e.preventDefault();
        
        if(!this.fileSelector) return;
        if(!this.fileSelector.files[0]) return;

        const file = this.fileSelector.files[0];

        const reader = new FileReader();


        const props = this.props;
        const execute = this.context.execute;

        reader.onload = function() {

            const oldBytearray = props.keyData.get('bytearray'),
                oldFilename = props.keyData.get('filename');


            const arr = new Uint8Array(reader.result);


            execute({
                doIt: () => {
                    props.keyData.set('bytearray', [PREFIX, ...arr]);
                    props.keyData.set('filename', file.name);
                },
                undoIt: () => {
                    props.keyData.set('bytearray', oldBytearray);
                    props.keyData.set('filename', oldFilename);
                },
                message: <span>Uploaded <code key={1}>{file.name}</code> to <code key={2}>{props.keyName}</code>.</span>,
                onSave: () => ({
                    [props.keyName]: props.keyData.get('bytearray')
                })
            });


        };

        reader.readAsArrayBuffer(file);

    }


    download() {

        const bytearray = this.props.keyData.get('bytearray');
        const arrBuffer = new Uint8Array(bytearray).buffer;

        const blob = new Blob([arrBuffer]);
        const link = document.createElement('a');
        link.href = window.URL.createObjectURL(blob);

        const fileName = this.props.keyData.get('filename')
            || this.props.keyName;

        link.download = fileName;
        link.click();

    }


    render() {

        const {keyData, keyName} = this.props;

        return (
            <div style={{ padding: 15 }}>
                <BS.Panel>
                    <BS.Panel.Heading>
                        File Editor
                    </BS.Panel.Heading>
                    <BS.Panel.Body>
                        <div>File size: <code><FileSize numBytes={keyData.get('bytearray').length - 1}/></code></div>
                        <div>File name: <code>{keyName}</code></div>

                        <hr/>

                        <BS.ListGroup>
                            <BS.ListGroupItem onClick={this.download.bind(this)}>
                                <BS.Glyphicon glyph='download'/> Download
                            </BS.ListGroupItem>
                        </BS.ListGroup>

                        <hr/>

                            <BS.FormGroup>
                                <input
                                    type='file'
                                    ref={el => this.fileSelector = el}/>
                                <BS.ListGroup>
                                    <BS.ListGroupItem onClick={this.onSubmit.bind(this)}>
                                        Submit
                                    </BS.ListGroupItem>
                                </BS.ListGroup>
                            </BS.FormGroup>

                        {
                            keyData.get('filename') &&

                            <div>
                                Uploaded <FileName filename={keyData.get('filename')}/> successfully.
                            </div>
                        }

                    </BS.Panel.Body>
                </BS.Panel>
            </div>
        );
    }

}


export const defaultKeyData = observable.map({
    bytearray: [PREFIX],
    filename: ''
});
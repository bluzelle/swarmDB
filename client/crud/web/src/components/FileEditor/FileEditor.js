import {FileName} from './FileName';
import {FileSize} from "./FileSize";
import {selectedKey} from '../KeyList';
import {activeValue} from '../../services/CRUDService';


@observer
export class FileEditor extends Component {


    constructor() {
        super();

        this.state = {

            uploaded: false,
            uploadedFilename: ''

        };

    }


    onSubmit(e) {

        e.preventDefault();
        
        if(!this.fileSelector) return;
        if(!this.fileSelector.files[0]) return;

        const file = this.fileSelector.files[0];

        const reader = new FileReader();


        reader.onload = () => {

            // const oldBytearray = props.keyData.get('bytearray'),
            ///    oldFilename = props.keyData.get('filename');


            activeValue.set(reader.result);

            this.setState({
                uploaded: true,
                uploadedFilename: file.name
            });


            // execute({
            //     doIt: () => {
            //         props.keyData.set('bytearray', [PREFIX, ...arr]);
            //         props.keyData.set('filename', file.name);
            //     },
            //     undoIt: () => {
            //         props.keyData.set('bytearray', oldBytearray);
            //         props.keyData.set('filename', oldFilename);
            //     },
            //     message: <span>Uploaded <code key={1}>{file.name}</code> to <code key={2}>{props.keyName}</code>.</span>
            // });


        };

        reader.readAsArrayBuffer(file);

    }


    download() {

        const arrBuffer = activeValue.get();

        const blob = new Blob([arrBuffer]);
        const link = document.createElement('a');
        link.href = window.URL.createObjectURL(blob);

        const fileName = this.state.uploadedFilename;

        link.download = fileName;
        link.click();

    }


    render() {

        return (
            <div style={{ padding: 15 }}>
                <BS.Panel>
                    <BS.Panel.Heading>
                        File Editor
                    </BS.Panel.Heading>
                    <BS.Panel.Body>
                        <div>File size: <code><FileSize numBytes={activeValue.get().byteLength}/></code></div>
                        <div>File name: <code>{selectedKey.get()}</code></div>

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
                            this.state.uploaded &&

                            <div>
                                Uploaded <FileName filename={this.state.uploadedFilename}/> successfully as <code>{selectedKey.get()}</code>.
                            </div>
                        }

                    </BS.Panel.Body>
                </BS.Panel>
            </div>
        );
    }

}
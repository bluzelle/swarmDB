import {getPrefix} from "./keyData";
import {JSONEditor, PREFIX as jsonPrefix} from "./JSONEditor";
import {PlainTextEditor, PREFIX as textPrefix} from './PlainTextEditor';
import {FileEditor, PREFIX as filePrefix} from "./FileEditor/FileEditor";
import {selectedKey} from "./KeyList";
import {sendToNodes} from "bluzelle-client-common/services/CommunicationService";


@observer
export class Editor extends Component {

    dataLoaded() {
        return this.props.obj.get(selectedKey.get()).has('bytearray');
    }

    loadData() {
        sendToNodes('requestBytearray', {key: selectedKey.get()});
    }

    render() {

        const {obj} = this.props;

        if(this.dataLoaded()) {
            return <EditorSwitch obj={obj}/>
        } else {
            this.loadData();
            return <Loading/>;
        }

    }
}


const Loading = () => (
    <div>
        Fetching data...
    </div>
);

const EditorSwitch = ({obj}) => {
    const keyData = obj.get(selectedKey.get());
    const type = getPrefix(keyData);

    return (
        <React.Fragment>
            {type === jsonPrefix && <JSONEditor keyData={keyData} keyName={selectedKey.get()}/>}
            {type === textPrefix &&
                <PlainTextEditor keyData={keyData} keyName={selectedKey.get()} key={selectedKey.get()}/>}
            {type === filePrefix &&
                <FileEditor keyData={keyData} keyName={selectedKey.get()} key={selectedKey.get()}/>}
        </React.Fragment>
    );
};
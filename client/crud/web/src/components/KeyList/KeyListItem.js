import {ObjIcon} from "../ObjIcon";
import {executeContext} from "../../services/CommandQueueService";
import {EditableField} from "../EditableField";
import {selectedKey} from "./KeyList";

@executeContext
@observer
export class KeyListItem extends Component {

    select(target) {
        const oldVal = selectedKey.get();

        this.context.execute({
            doIt: () => selectedKey.set(target),
            undoIt: () => selectedKey.set(oldVal),
            onSave: () => {},
            message: <span>Selected <code key={1}>{target}</code>.</span>
        });
    }

    rename(newKey) {

        const {obj, keyname: keyName} = this.props;

        selectedKey.get() === keyName ? changeCurrentSelection.call(this) : changeNoncurrentSelection.call(this);


        function renameInObj(obj, oldKey, newKey) {
            obj.set(newKey, obj.get(oldKey));
            obj.delete(oldKey);
        }

        function onSave(savedKeys) {
            return {
                [newKey]: savedKeys[keyName] || obj.get(newKey).get('bytearray'),
                [keyName]: 'deleted'
            };
        }

        function message() {
            return <span>Renamed <code key={1}>{keyName}</code> to <code key={2}>{newKey}</code>.</span>;
        }


        function changeCurrentSelection() {
            this.context.execute({
                doIt: () => {
                    selectedKey.set(null);
                    renameInObj(obj, keyName, newKey);
                    selectedKey.set(newKey);
                },
                undoIt: () => {
                    selectedKey.set(null);
                    renameInObj(obj, newKey, keyName);
                    selectedKey.set(keyName);
                },
                onSave,
                message: message()
            });
        }

        function changeNoncurrentSelection() {
            this.context.execute({
                doIt: () => renameInObj(obj, keyName, newKey),
                undoIt: () => renameInObj(obj, newKey, keyName),
                onSave,
                message: message()
            });
        }

    }


    render() {

        const {obj, keyname} = this.props;

        return (
            <BS.ListGroupItem
                onClick={() => selectedKey.get() === keyname ? this.select(null) : this.select(keyname)}
                active={selectedKey.get() === keyname}>

                <span style={{display: 'inline-block', width: 25}}>
                    <ObjIcon keyData={obj.get(keyname)}/>
                </span>

                <EditableField
                    val={keyname}
                    onChange={this.rename.bind(this)}/>

                {
                    keyname === selectedKey.get() ?
                        <BS.Glyphicon
                            style={{float: 'right'}}
                            glyph='chevron-right'/>
                        : null
                }
            </BS.ListGroupItem>);
    }
}
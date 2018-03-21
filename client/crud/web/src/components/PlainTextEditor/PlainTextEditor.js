import {getRaw, addPrefix} from "../keyData";
import {arrayToStr, strToArray} from "../../util/encoding";
import {enableExecution} from "../../services/CommandQueueService";

export const PREFIX = 1;

@observer
@enableExecution
export class PlainTextEditor extends Component {

    constructor(props) {
        super(props);

        const {keyData} = this.props;

        const val = keyData.has('localChanges') ? keyData.get('localChanges') : interpret(keyData);
        keyData.has('localChanges') || keyData.set('beginEditingTimestamp', new Date().getTime());

        this.state = {val};

        keyData.set('localChanges', val);
    }

    onSubmit(e) {
        e && e.preventDefault();

        const {keyName, keyData} = this.props;

        const oldVal = keyData.get('localChanges');
        const newVal = this.state.val;


        this.context.execute({
            doIt: () => keyData.set('localChanges', newVal),
            undoIt: () => keyData.set('localChanges', oldVal),
            onSave: this.onSave.bind(this, newVal),
            message: <span>Updated <code key={1}>{keyName}</code>.</span>
        });
    }

    onSave(interpreted) {
        return {
            [this.props.keyName]: serialize(interpreted)
        };
    }

    onChange(e) {
        this.setState({val: e.target.value});
    }

    render() {
        return (
            <div style={{height: '100%'}}>
                <BS.Form style={{height: '100%'}}>
                    <BS.FormControl
                        style={{height: '100%', resize: 'none'}}
                        componentClass="textarea"
                        value={this.state.val}
                        onChange={this.onChange.bind(this)}
                        onBlur={this.onSubmit.bind(this)}/>
                </BS.Form>
            </div>
        );
    }
}

export const textToKeyData = str => observable.map({
    bytearray: serialize(str)
});

const serialize = str => addPrefix(strToArray(str), PREFIX);

export const interpret = keyData =>
    arrayToStr(getRaw(keyData));

export const defaultKeyData = () => textToKeyData('');
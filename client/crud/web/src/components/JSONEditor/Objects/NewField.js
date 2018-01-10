import {observableMapRecursive} from "../../../util/mobXUtils";
import {EditableField} from "../../EditableField";

export class NewField extends Component {
    constructor(props) {
        super(props);

        this.state = {
            currentInput: 'key',
            key: 'key'
        };
    }

    render() {
        const {onChange, onError} = this.props;

        const keyField =
            <EditableField
                active={this.state.currentInput === 'key'}
                val={this.state.key}
                onChange={key => {
                    this.setState({ currentInput: 'val', key });
                }}/>;

        const valField =
            <EditableField
                active={this.state.currentInput === 'val'}
                val={'"value"'}
                validateJSON={true}
                onChange={val => {
                    try {
                        const obj = observableMapRecursive(JSON.parse(val));
                        onChange(this.state.key, obj);
                    } catch(e) {
                        onError();
                    }
                }}/>;

        return (
            <div>
                {keyField}:{valField}
            </div>
        );
    }
}
import {NewKeyTypeModal} from "./NewKeyTypeModal";
import {EditableField} from "../EditableField";

export class NewKeyField extends Component {

    constructor(props) {
        super(props);

        this.state = {
            showModal: false,
            keyField: ''
        };
    }

    onChange(key) {

        this.setState({ keyField: key });
        isEmpty() ? this.exit() : this.showModal();

        function isEmpty() {
            return key === '';
        }
    }

    showModal() {
        this.setState({ showModal: true });
    }

    exit() {
        this.props.onChange();
    }

    render() {
        const {obj} = this.props;

        return (
            <React.Fragment>
                <BS.ListGroupItem>
                    <span style={{display: 'inline-block', width: 25}}>
                        <BS.Glyphicon glyph='asterisk'/>
                    </span>
                    <EditableField
                        val={this.state.keyField}
                        active={true}
                        onChange={this.onChange.bind(this)}/>
                </BS.ListGroupItem>

                {this.state.showModal &&
                    <NewKeyTypeModal
                        onHide={this.exit.bind(this)}
                        obj={obj}
                        keyField={this.state.keyField}/>}
            </React.Fragment>
        );
    }
}
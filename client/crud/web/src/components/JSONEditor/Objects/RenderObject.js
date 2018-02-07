import {Collapsible} from "../Collapsible";
import {Plus, Edit, Delete} from "../Buttons";
import {Hoverable} from '../Hoverable.js';
import {get} from '../../../util/mobXUtils';
import {RenderTreeWithEditableKey} from "./RenderTreeWithEditableKey";
import {NewField} from "./NewField";
import {del, enableExecution} from '../../../services/CommandQueueService';

@enableExecution
@observer
export class RenderObject extends Component {
    constructor(props) {
        super(props);

        this.state = {
            showNewField: false
        };
    }

    render() {
        const {obj, propName, preamble, hovering, onEdit, isRoot} = this.props;

        const buttons = hovering &&
            <React.Fragment>
                <Plus onClick={() => this.setState({showNewField: true})}/>
                {isRoot || <Delete onClick={() => del(this.context.execute, obj, propName)}/>}
                <Edit onClick={onEdit}/>
            </React.Fragment>;


        const newField = this.state.showNewField &&
            <Hoverable>
                <NewField
                    onChange={(key, val) => {
                        this.setState({showNewField: false});

                        this.context.execute({
                            doIt: () => get(obj, propName).set(key, val),
                            undoIt: () => get(obj, propName).delete(key),
                            message: <span>New field <code key={1}>{key}</code>: <code key={2}>{JSON.stringify(val)}</code>.</span>});
                    }}
                    onError={() => this.setState({showNewField: false})}/>
            </Hoverable>;


        const fieldList = get(obj, propName).keys().sort().map(subkey =>
            <RenderTreeWithEditableKey
                key={subkey}
                obj={get(obj, propName)}
                propName={subkey}/>);


        return (
            <Collapsible
                label={`{} (${get(obj, propName).keys().length} entries)`}
                buttons={buttons}
                preamble={preamble}>

                {newField}
                {fieldList}
            </Collapsible>
        );
    }
}
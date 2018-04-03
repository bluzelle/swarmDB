import {RenderTree} from "../Trees/RenderTree";
import {Collapsible} from "../Collapsible";
import {Plus, Edit, Delete} from "../Buttons";
import {Hoverable} from "../Hoverable";
import {NewField} from "./NewField";
import {observableMapRecursive as omr} from '../JSONEditor';
import {execute} from '../../../services/CommandQueueService';


@observer
export class RenderArray extends Component {
    constructor(props) {
        super(props);

        this.state = {
            showNewField: false
        };
    }

    render() {
        const {val, set, del, preamble, hovering, onEdit} = this.props;

        const buttons = hovering &&
            <React.Fragment>
                <Plus onClick={() => this.setState({ showNewField: true })}/>
                {del && <Delete onClick={() => del()}/>}
                <Edit onClick={onEdit}/>
            </React.Fragment>;


        const newField = this.state.showNewField &&
            <Hoverable>
                <NewField
                    preamble={val.length}
                    onChange={newObj => {

                        this.setState({ showNewField: false });

                        const v2 = omr(newObj);

                        execute({
                            doIt: () => Promise.resolve(val.push(v2)),
                            undoIt: () => Promise.resolve(val.pop()),
                            message: <span>Pushed <code key={1}>{JSON.stringify(newObj)}</code>.</span>});
                    }}

                    onError={() => this.setState({showNewField: false})}/>

            </Hoverable>;


        const fieldList = val.map((value, index) =>
            <RenderTree
                key={index}

                val={value}
                set={v => {

                    execute({
                        doIt: () => Promise.resolve(val[index] = v),
                        undoIt: () => Promise.resolve(val[index] = value),
                        message: <span>Set index <code key={1}>{index}</code> to <code key={2}>{JSON.stringify(v)}</code>.</span>
                    });

                }}

                del={() => {

                    execute({
                        doIt: () => Promise.resolve(val.splice(index, 1)),
                        undoIt: () => Promise.resolve(val.splice(index, 0, value)),
                        message: <span>Deleted index <code key={1}>{index}</code>.</span>
                    });
                    
                }}

                preamble={<span>{index}</span>}/>);


        return <Collapsible
            label={`[] (${val.length} entries)`}
            buttons={buttons}
            preamble={preamble}>

            {fieldList}
            {newField}
        </Collapsible>;
    }
}
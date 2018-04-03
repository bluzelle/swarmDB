import {KeyListItem} from "./KeyListItem";
import {NewKeyField} from "./NewKey/NewKeyField";
import {activeValue, save, remove, reload} from '../../services/CRUDService';
import {execute, removePreviousHistory, updateHistoryMessage} from '../../services/CommandQueueService';

import {update, keys as bzkeys} from 'bluzelle';


export const selectedKey = observable(undefined);


const keys = observable([]);

export const refreshKeys = () => 
    bzkeys().then(k => keys.replace(k));



@observer
export class KeyList extends Component {

    constructor(props) {
        super(props);

        this.state = {
            showNewKey: false
        };

    }


    componentWillMount() {

        refreshKeys();

    }


    render() {

        const keyList = keys.sort().map(keyname =>
            <KeyListItem key={keyname} keyname={keyname}/>);

        return (
            <div style={{padding: 10}}>
                <BS.ListGroup>

                    {keyList}

                    { this.state.showNewKey &&
                        <NewKeyField onChange={() => this.setState({showNewKey: false})}/> }
                
                </BS.ListGroup>


                <BS.ButtonToolbar>
                    <BS.ButtonGroup>

                        <AddButton onClick={() => this.setState({showNewKey: true})}/>
                        
                        {

                            activeValue.get() !== undefined &&

                            <BS.Button onClick={executeRemove} style={{color: 'red'}}>
                                <BS.Glyphicon glyph='remove'/>
                            </BS.Button>

                        }

                    </BS.ButtonGroup>

                    <SaveReloadRemove/>
                </BS.ButtonToolbar>
            </div>
        );
    }
}


const executeRemove = () => {

    const sk = selectedKey.get();
    const val = activeValue.get();


    execute({

        doIt: () => remove(),

        undoIt: () => new Promise(resolve =>

            update(sk, val).then(() => reload().then(() => {

                selectedKey.set(sk);
                resolve();

            }))),

        message: <span>Removed key <code key={1}>{sk}</code>.</span>

    });

};


// Doesn't play nice with the current system of undos.

// An idea: track changes to observables and automatically 
// generate undo/redo behavior based on single command.

const executeReload = () => {

    reload();

    removePreviousHistory();
    updateHistoryMessage('Reload');

};


const AddButton = ({onClick}) => 

    <BS.Button onClick={onClick} style={{color: 'green'}}>
        <BS.Glyphicon glyph='plus'/>
    </BS.Button>;


const SaveReloadRemove = observer(({keyname}) =>

        <BS.ButtonGroup>
           <BS.Button onClick={executeReload} style={{color: 'DarkBlue'}}>
                <BS.Glyphicon glyph='refresh'/>
            </BS.Button>

            {

                activeValue.get() !== undefined &&
                
                <BS.Button onClick={save} style={{color: 'DarkGreen'}}>
                    <BS.Glyphicon glyph='floppy-save'/>
                </BS.Button>

            }

        </BS.ButtonGroup>);

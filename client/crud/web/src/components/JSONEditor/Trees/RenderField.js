import {get, observableMapRecursive} from "../../../util/mobXUtils";
import {EditableField} from "../../EditableField";
import {Delete} from "../Buttons";
import {del, executeContext} from "../../../services/CommandQueueService";

export const RenderField =
    executeContext(({obj, propName, preamble, editing, onChange, hovering}, context) => (

        <div>
            {preamble && <span style={{marginRight: 5}}>{preamble}:</span>}

            <EditableField
                active={editing}
                onChange={v => {
                    onChange();

                    const oldVal = get(obj, propName);
                    const newVal = observableMapRecursive(JSON.parse(v));

                    context.execute({
                        doIt: () => obj.set(propName, newVal),
                        undoIt: () => obj.set(propName, oldVal),
                        message: <span>Set <code key={1}>{propName}</code> to <code key={2}>{v}</code>.</span>
                    });
                }}
                val={JSON.stringify(get(obj, propName))}
                validateJSON={true}
                renderVal={v =>
                    <span style={{color: colorFromType(v)}}>{v}</span>}/>

            {hovering && <Delete onClick={() => del(context.execute, obj, propName)}/>}
        </div>
    ));

const colorTypeMap = {
    string: 'blue',
    number: 'red',
    boolean: 'purple'
};

const colorFromType = obj =>
    colorTypeMap[typeof JSON.parse(obj)] || 'pink';
import {get, observableMapRecursive} from "../../../util/mobXUtils";
import {EditableField} from "../../EditableField";
import {Delete} from "../Buttons";
import {observableMapRecursive as omr} from '../JSONEditor';

export const RenderField =
    ({val, set, del, preamble, editing, onChange, hovering}) => (

        <div>
            {preamble && <span style={{marginRight: 5}}>{preamble}:</span>}

            <EditableField
                active={editing}
                onChange={v => {

                    onChange();

                    // const oldVal = get(obj, propName);
                    // const newVal = observableMapRecursive(JSON.parse(v));

                    // obj.set(propName, newVal)

                    set(omr(JSON.parse(v)));

                    // context.execute({
                    //     doIt: () => ,
                    //     undoIt: () => obj.set(propName, oldVal),
                    //     message: <span>Set <code key={1}>{propName}</code> to <code key={2}>{v}</code>.</span>
                    // });
                }}

                val={JSON.stringify(val)}

                validateJSON={true}

                renderVal={v =>
                    <span style={{color: colorFromType(v)}}>{v}</span>}/>

            {hovering && del && <Delete onClick={() => del()}/>}

        </div>
    );


const colorTypeMap = {
    string: 'blue',
    number: 'red',
    boolean: 'purple'
};


const colorFromType = obj =>
    colorTypeMap[typeof JSON.parse(obj)] || 'pink';
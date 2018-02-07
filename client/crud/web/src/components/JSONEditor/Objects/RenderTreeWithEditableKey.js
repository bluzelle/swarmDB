import {EditableField} from "../../EditableField";
import {RenderTree} from "../Trees/RenderTree";
import {enableExecution} from "../../../services/CommandQueueService";

export const RenderTreeWithEditableKey =
    enableExecution(({obj, propName, ...props}, context) => {
        const preamble =
            <EditableField
                val={propName}
                renderVal={val => <span style={{color: 'navy'}}>{val}</span>}
                onChange={newkey => {

                    context.execute({
                        doIt: () => {
                            const oldval = obj.get(propName);
                            obj.delete(propName);
                            obj.set(newkey, oldval);
                        },
                        undoIt: () => {
                            const oldval = obj.get(newkey);
                            obj.delete(newkey);
                            obj.set(propName, oldval);
                        },
                        message: <span>Renamed <code key={1}>{propName}</code> to <code key={2}>{newkey}</code>.</span>
                    });
                }}/>;

        return <RenderTree
            obj={obj}
            propName={propName}
            preamble={preamble}
            {...props}/>;
    });
import {EditableField} from "../../EditableField";
import {RenderTree} from "../Trees/RenderTree";

export const RenderTreeWithEditableKey =
    ({...props}) => {


        // Not sure how to get this working...

        // const preamble =

        //     <EditableField
        //         val={val}
        //         renderVal={val => <span style={{color: 'navy'}}>{val}</span>}
        //         onChange={newkey => {

                    // const oldval = val;
                    // obj.delete(propName);
                    // obj.set(newkey, oldval);


                    // Not sure how this will work

                    // del();
                    // set();


                    // context.execute({
                    //     doIt: () => {
                    //         const oldval = obj.get(propName);
                    //         obj.delete(propName);
                    //         obj.set(newkey, oldval);
                    //     },
                    //     undoIt: () => {
                    //         const oldval = obj.get(newkey);
                    //         obj.delete(newkey);
                    //         obj.set(propName, oldval);
                    //     },
                    //     message: <span>Renamed <code key={1}>{propName}</code> to <code key={2}>{newkey}</code>.</span>
                    // });
         //       }}/>;

        return <RenderTree {...props}/>;

    };
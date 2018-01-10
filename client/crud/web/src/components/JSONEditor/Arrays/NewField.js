import {EditableField} from "../../EditableField";
import {observableMapRecursive} from "../../../util/mobXUtils";

export const NewField = ({ preamble, onChange, onError }) => (
    <div>
        {preamble}:

        <EditableField
            active={true}
            val={''}
            validateJSON={true}
            onChange={val => {
                try {
                    const obj = observableMapRecursive(JSON.parse(val));
                    onChange(obj);
                } catch(e) {
                    onError();
                }
            }}/>
    </div>
);
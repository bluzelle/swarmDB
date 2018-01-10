import {RenderTree} from "./Trees/RenderTree";
import {pipe} from 'lodash/fp';
import {getRaw, addPrefix} from "../keyData";
import {observableMapRecursive as omr} from "../../util/mobXUtils";
import {byteArrayToStr, strToByteArray} from "../../util/encoding";
import {executeContext, setExecuteContext} from "../../services/CommandQueueService";

export const PREFIX = 0;

@executeContext
@setExecuteContext
export class JSONEditor extends Component {

    getChildContext() {
        const {keyData} = this.props;

        return {
            execute: args => this.context.execute({
                onSave: this.onSave.bind(this, keyData.get('interpreted')), ...args })
        };
    }

    onSave(interpreted) {
        return {
            [this.props.keyName]: addPrefix(serialize(interpreted), PREFIX)
        };
    }

    render() {
        const {keyData} = this.props;

        keyData.has('interpreted')
            || keyData.set('interpreted', omr(interpret(getRaw(keyData))));

        return <RenderTree obj={keyData} propName='interpreted' isRoot={true}/>
    }
}


const interpret = pipe(byteArrayToStr, JSON.parse);
const serialize = pipe(JSON.stringify, strToByteArray);


export const objectToKeyData = obj => ({
    bytearray: addPrefix(serialize(obj), PREFIX)});

export const defaultKeyData = objectToKeyData({});
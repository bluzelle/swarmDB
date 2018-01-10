import {getPrefix} from "./keyData";
import {PREFIX as textPrefix} from "./PlainTextEditor";
import {PREFIX as jsonPrefix} from "./JSONEditor";

export const ObjIcon = observer(({keyData}) => (
    <span style={{display: 'inline-block', width: 25}}>
        { getPrefix(keyData) === jsonPrefix &&
        <span style={{
            fontWeight: 'bold',
            fontFamily: 'monospace'
        }}>{'{}'}</span> }

        { getPrefix(keyData) === textPrefix &&
        <BS.Glyphicon glyph='font'/> }
    </span>
));
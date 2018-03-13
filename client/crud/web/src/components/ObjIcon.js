import {getPrefix} from "./keyData";
import {PREFIX as textPrefix} from "./PlainTextEditor";
import {PREFIX as jsonPrefix} from "./JSONEditor";
import {PREFIX as filePrefix} from "./FileEditor/FileEditor";

export const ObjIcon = observer(({keyData}) => (
    <span style={{display: 'inline-block', width: 25}}>
        { getPrefix(keyData) === jsonPrefix &&
            <span style={{
                fontWeight: 'bold',
                fontFamily: 'monospace'
            }}>{'{}'}</span> }

        { getPrefix(keyData) === textPrefix &&
            <BS.Glyphicon glyph='font'/> }


        { getPrefix(keyData) === filePrefix &&
            <BS.Glyphicon glyph='file'/> }
    </span>
));
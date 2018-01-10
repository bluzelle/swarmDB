const common = {
    border: 0,
    background: 'none'
};

export const Delete = ({ onClick }) => (
    <BS.Button
        bsSize='xsmall'
        style={{
            ...common,
            float: 'right',
            color: 'red'
        }} onClick={ onClick }>
        <BS.Glyphicon glyph='remove'/>
    </BS.Button>
);


export const Edit = ({ onClick }) => (
    <BS.Button
        bsSize='xsmall'
        style={{
            ...common,
            float: 'right',
            color: 'orange'
        }} onClick={ onClick }>
        <BS.Glyphicon glyph='pencil'/>
    </BS.Button>
);

export const Plus = ({ onClick }) => (
    <BS.Button
        bsSize='xsmall'
        style={{
            ...common,
            color: 'green'
        }} onClick={ onClick }>
        <BS.Glyphicon glyph='plus'/>
    </BS.Button>
);
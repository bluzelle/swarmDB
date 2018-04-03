export const TextIcon = () => 
	<BS.Glyphicon glyph='font'/>;

export const FileIcon = () => 
	<BS.Glyphicon glyph='file'/>;

export const JSONIcon = () =>
    <span style={{
        fontWeight: 'bold',
        fontFamily: 'monospace'
    }}>
        {'{}'}
    </span>;


export const ValIcon = ({val}) => {

	if(typeof val === 'string') {

		return <TextIcon/>;

	}


	if(typeof val === 'object') {

		return <JSONIcon/>;

	}

	return <BS.Glyphicon glyph='question-sign'/>;

};
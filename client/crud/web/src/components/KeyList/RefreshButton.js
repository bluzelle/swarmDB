export const RefreshButton = ({keyData}) => {
    return (
        <BS.OverlayTrigger placement="bottom" overlay={
            <BS.Tooltip id="refresh-tooltip">
                Began editing at: {new Date(keyData.get('beginEditingTimestamp')).toLocaleTimeString()}.
                <br/>
                Most recent version: {new Date(keyData.get('mostRecentTimestamp')).toLocaleTimeString()}.
            </BS.Tooltip>
        }>
            <div style={{
                display: 'inline-block',
                marginRight: 8
            }}>
                <BS.Glyphicon glyph='refresh'
                              onClick={(e) => {
                                  e.stopPropagation();
                                  keyData.clear();
                              }}
                              style={{
                                  verticalAlign: 'middle'
                              }}/>
            </div>
        </BS.OverlayTrigger>
    );
};
import statusColors from 'constants/nodeStatusColors';


const StatusFormatter = ({value}) => (
    <div style={{lineHeight: '15px'}}>
        <svg style={{marginRight: 8}} width="15" height="15">
            <rect width="15" height="15" fill={statusColors[value]}/>
        </svg>
        {value}
    </div>
);

export default StatusFormatter

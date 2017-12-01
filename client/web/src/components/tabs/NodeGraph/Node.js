import nodeColors from 'constants/nodeStatusColors'
import Star from 'components/Star'

const Node = ({node, onMouseOver, selected}) => {
    const {address, nodeState, xAngle, yAngle} = node;
    const cx = 90 * xAngle + 100;
    const cy = 90 * yAngle + 100;

    const r = new Date().getTime() - node.lastMessageReceived < 700 ? 6 : 4;
    const circumference = r * 3.14;
    const percent = node.used / node.available;
    return (
        <g data-test={`node-${address.replace(':', '-')}`} onMouseOver={onMouseOver}>
            <circle  cx={cx} cy={cy} r={r + 1} fill="#444" style={{
                stroke: '#666',
                strokeWidth: 1
            }}/>
            <circle
                style={{
                    fill: '#444',
                    strokeDasharray: `${circumference * percent},${circumference * (1 - percent)}`,
                    stroke:  nodeColors[nodeState],
                    strokeWidth: r
                }}
                key={`circle-${address}`}
                cx={cx}
                cy={cy}
                r={r / 2}/>
            {selected && <circle fill='white' key={`circle-border=${address}`} cx={cx} cy={cy} r="2"/>}
            {node.isLeader && <Star {...{cx, cy, r}} />}
        </g>
    )
};

export default observer(Node);


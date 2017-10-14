import nodeColors from 'constants/nodeStatusColors'

const Node = observer(({node, onMouseOver, selected}) => {
    const {address, nodeState, xAngle, yAngle} = node;
    const cx = 90 * xAngle + 100;
    const cy = 90 * yAngle + 100;
    return [
        selected && <circle fill='white' stroke="black" strokeWidth="1" key={`circle-border=${address}`} cx={cx} cy={cy} r="4"/>,
        <circle fill={nodeColors[nodeState]} onMouseOver={onMouseOver} key={`circle-${address}`} cx={cx} cy={cy} r="3"/>
    ];
});

export default Node


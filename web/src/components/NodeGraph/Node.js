const Node = ({address, messages, xAngle, yAngle, onMouseOver, selected}) => {
    const cx = 90 * xAngle + 100;
    const cy = 90 * yAngle + 100;

    return [
        selected && <circle fill='white' stroke="red" strokeWidth="1" key={`circle-border=${address}`} cx={cx} cy={cy} r="4"/>,
        <circle onMouseOver={onMouseOver} key={`circle-${address}`} cx={cx} cy={cy} r="3"/>
    ];
};

export default Node
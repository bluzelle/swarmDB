import range from 'lodash/range'

const Star = ({cx, cy, r}) => {
    const points = range(0, 10)
        .map(i => (Math.PI * 2 / 10) * i)
        .map((a, idx) => {
            const offset = (((idx % 2) ? .5 : 1) * r) / 2;
            return [Math.cos(a) * offset + cx, Math.sin(a) * offset + cy]
        })
        .map(([x, y]) => `${x},${y}`)
        .join(' ');
    return (
        <polygon points={points} fill="white"/>
    );

};

export default Star
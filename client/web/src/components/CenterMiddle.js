const CenterMiddle = ({children}) => (
    <div style={wrapperStyle}>
        <div style={innerStyle}>
            {children}
        </div>
    </div>
);

export default CenterMiddle
const wrapperStyle = {
    height: '100%',
    width: '100%',
    position: 'relative'
};

const innerStyle = {
    position: 'absolute',
    top: '50%',
    left: '50%',
    padding: 15,
    transform: 'translate(-50%,-50%)'
};
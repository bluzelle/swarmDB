const PercentageDisplay = ({part,total}) => (
        <span>
            {Math.round(part/total*1000)/10}%
        </span>
);

export default PercentageDisplay
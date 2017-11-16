const StorageDisplay = ({size}) => (
    <span>
        {toUnit(size)}
    </span>
);

export default StorageDisplay

const toUnit = (size) => size < 1024 ? `${size} MB` : `${(size/1024).toFixed(1)} GB`;
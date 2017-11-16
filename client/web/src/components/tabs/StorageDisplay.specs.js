import StorageDisplay from './StorageDisplay'

describe('components/tab/StorageDisplay', () => {
    it('should show size in MB', () => {
        testIt(50, /50 MB/)
    });

    it('should show size in GB', () => {
        testIt(50*1024, /50\.0 GB/)
    });

    it('should handle fractional input and show GB', () => {
        testIt(49*1024, /49\.0 GB/)
    });

    it('should handle fractional output and show GB', () => {
        testIt(49.7*1024, /49\.7 GB/)
    });

    it('should handle zero', () => {
        testIt(0, /0 MB/)
    });

    const testIt = (size, output) => {
        const wrapper = shallow(<StorageDisplay size={size} />);
        expect(wrapper).to.have.html().match(output);
    }

});
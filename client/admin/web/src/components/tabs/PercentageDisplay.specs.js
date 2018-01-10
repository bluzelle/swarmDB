import PercentageDisplay from './PercentageDisplay'

describe('components/tab/PercentageDisplay', () => {
    it('should show size in percentage', () => {
        testIt(100, 50, /50\%/)
    });

    it('should show size in percentage', () => {
        testIt(100*1024, 50*1024, /50\%/)
    });

    it('should handle fractional input and show percentage', () => {
        testIt(100*1024, 50173, /49\%/)
    });

    it('should handle fractional output and show percentage', () => {
        testIt(100*1024, 50873, /49\.7\%/)
    });

    it('should handle zero used', () => {
        testIt(100, 0, /0\%/)
    });

    it('should handle 100% used', () => {
        testIt(24376, 24376, /100\%/)
    });

    const testIt = (total, part, output, noPercent) => {
        const wrapper = shallow(<PercentageDisplay total={total} part={part} />);
        expect(wrapper).to.have.html().match(output);
    }

});


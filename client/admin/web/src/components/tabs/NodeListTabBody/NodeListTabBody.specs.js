import NodeList from './NodeListTabBody'

describe('components/NodeListTabBody', () => {
    it('should render without errors if there are no nodes', () => {
        const wrapper = mount(<NodeList/>);
        expect(wrapper).to.have.html().match(/Address/);
        wrapper.unmount();
    });
});
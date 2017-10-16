import NodeGraph from './NodeGraph'

describe('components/NodeGraph', () => {
    it('should render without errors if there are no nodes', () => {
        const wrapper = shallow(<NodeGraph/>)
        expect(wrapper).to.have.html().match(/0 Nodes/);
        wrapper.unmount();
    });
});
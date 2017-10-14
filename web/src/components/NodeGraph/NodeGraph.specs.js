import NodeGraph from './NodeGraph'

describe('components/NodeGraph/NodeGraphView', () => {
    it('should render without errors if there are no nodes', () => {
        const wrapper = shallow(<NodeGraph nodes={[]}/>)
        expect(wrapper).to.have.html().match(/0 Nodes/);
    });
});
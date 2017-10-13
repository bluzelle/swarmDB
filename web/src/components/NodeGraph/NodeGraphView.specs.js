import NodeGraphView from './NodeGraphView'

describe('components/NodeGraph/NodeGraphView', () => {
    it('should render without errors if there are no nodes', () => {
        const wrapper = render(<NodeGraphView nodes={[]}/>)
        expect(wrapper).to.have.html().match(/0 Nodes/);
    })

    it('should render with multiple nodes', () => {
        const nodes = [{address: 1, messages: 1, xAngle: 1, yAngle: 1}, {address: 2, messages: 1, xAngle: 1, yAngle: 1}];

        const wrapper = render(<NodeGraphView nodes={nodes} />)
        expect(wrapper).to.have.html().match(/2 Nodes/);
    })
});
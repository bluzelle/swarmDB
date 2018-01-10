import Node from './Node'

describe('tabs/NodeGraph/Node', () => {
    it('should mount without errors', () => {
        const wrapper = render(<Node node={{address: '1', xAngle: 0, yAngle: 0}} onMouseOver={() => {
        }} selected={false}/>);
    });

    it('should not have a second circle when not selected', () => {
        const wrapper = render(<Node node={{address: '1', xAngle: 0, yAngle: 0}} onMouseOver={() => {
        }} selected={false}/>);
        expect(wrapper).to.have.exactly(2).descendants('circle');
    });

    it('should include another circle when selected', () => {
        const wrapper = mount(<Node node={{address: '1', xAngle: 0, yAngle: 0}} onMouseOver={() => {
        }} selected={true}/>);
        expect(wrapper).to.have.exactly(3).descendants('circle');
        wrapper.unmount()
    });
});
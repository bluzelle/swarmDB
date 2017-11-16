import NodeInfo from './NodeInfo'
import {updateNode, clearNodes, getNodeByAddress} from "../../services/NodeService";
import {MemoryRouter} from 'react-router-dom'

describe('components/tabs/NodeInfo', () => {
// test not working due to the node service not having node
    // it.only('should render a table showing node information', () => {
    //     const node = {address: '0xff', nodeState: 'alive', messages: 100};
    //
    //     const wrapper = shallow(<NodeInfo node={node} />);
    //
    //     console.log(wrapper.find('td').map(x => x));
    //
    //     expect(wrapper.find('td').map(x => x.text())).to.deep.equal(['0xff', '100', 'alive'])
    //
    // })

    let wrapper;
    let node;
    beforeEach(async () => {
        clearNodes();
        const node = {address: '0x01', available: 50, used: 10};
        updateNode(node);
        await waitFor(() => getNodeByAddress(node.address));
        wrapper = mount(<MemoryRouter><NodeInfo node={node}/></MemoryRouter>);
    });

    afterEach(() => {
        wrapper.unmount();
    });

    it('should display node address', () => {
        expectTDWith(wrapper, '0x01');
    });

    it('should show node messages when message button clicked', () => {
        expect(wrapper.find('a[href="/message-list/filtered-by-address/0x01"]').exists()).to.equal(true);
    });

    it('should update status', () => {
        expectTDWith(wrapper, 'new');
    });

    it('should show node available space', async () => {
        expectTDWith(wrapper, '50 MB');
        updateNode({address: '0x01', available: 60});
        await waitFor(() => wrapper.html().includes('60 MB'))
    });

    it('should show node space used in MB or GB and as a percentage', async () => {
        expectTDWith(wrapper, '10 MB.*(.*20%.*)');
        updateNode({address: '0x01', used: 2175});
        await waitFor(() => wrapper.html().includes('2.1 GB'))
    });

    const expectTDWith = (wrapper, string) =>
        expect(wrapper).to.have.html().match(new RegExp(`<td[^>]*>.*${string}`));

});
import NodeInfo from './NodeInfo'
import {updateNode, clearNodes, getNodeByAddress} from "../../services/NodeService";
import {MemoryRouter} from 'react-router-dom'

describe('components/tabs/NodeInfo', () => {

    let wrapper;
    let node;
    beforeEach(async () => {
        clearNodes();
        const node = {ip: '1.2.3', port: 1, available: 50, used: 10};
        updateNode(node);
        await waitFor(() => getNodeByAddress('1.2.3:1'));
        wrapper = mount(<MemoryRouter><NodeInfo node={node}/></MemoryRouter>);
    });

    afterEach(() => {
        wrapper.unmount();
    });

    it('should display node address', () => {
        expectTDWith(wrapper, '1.2.3:1');
    });

    it('should show node messages when message button clicked', () => {
        expect(wrapper.find('a[href="/message-list/filtered-by-address/1.2.3:1"]').exists()).to.equal(true);
    });

    it('should update status', () => {
        expectTDWith(wrapper, 'new');
    });

    it('should show node available space', async () => {
        expectTDWith(wrapper, '50 MB');
        updateNode({ip: '1.2.3', port: 1, available: 60});
        await waitFor(() => wrapper.html().includes('60 MB'))
    });

    it.only('should show node space used in MB or GB and as a percentage', async () => {
        expectTDWith(wrapper, '10 MB.*(.*20%.*)');
        updateNode({ip: '1.2.3', port: 1, used: 2175});

        await waitFor(() => wrapper.html().includes('2.1 GB'))
    });

    const expectTDWith = (wrapper, string) =>
        expect(wrapper).to.have.html().match(new RegExp(`<td[^>]*>.*${string}`));

});
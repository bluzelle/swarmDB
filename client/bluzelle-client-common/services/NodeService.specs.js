import {updateNode, getNodes, clearNodes, removeNodeByAddress} from './NodeService'
import {toJS} from 'mobx'
describe('services/NodeService', () => {

    beforeEach(clearNodes);

    describe('getNodes()', () => {
        it('should get nodes', async () => {
            updateNode({address: 1});
            await waitFor(() => getNodes().length === 1);
        })
    });

    describe('updateNode()', () => {
        it('should add nodes and trigger reactive', async () => {
            const spy = sinon.spy();
            autorun(() => {spy(getNodes().map(x => x))});
            expect(spy).to.have.been.calledOnce;
            expect(spy).to.have.been.calledWith([]);

            spy.reset();
            updateNode({ ip: '1', port: 1  });

            await waitFor(() => getNodes().length === 1);
            expect(spy).to.have.been.calledWith(sinon.match(v => v[0].port === 1));

            spy.reset();
            updateNode({ ip: '1', port: 2 });
            await waitFor(() => getNodes().length === 2);
            expect(spy).to.have.been.calledWith(sinon.match(v => v[1].port === 2));
        })
    });

    describe('removeNodeByAddress()', () => {
        it('should remove a node given the correct address', async () => {
            updateNode({ip:'1.2.3', port:1});
            updateNode({ip:'3.2.1', port:7});
            updateNode({ip:'192.168.0.1', port:8080});

            await waitFor(() => getNodes().length === 3);

            removeNodeByAddress('99.99.99:5');

            await waitFor(() => getNodes().length === 3);

            removeNodeByAddress('1.2.3:1');
            removeNodeByAddress('3.2.1:7');
            removeNodeByAddress('192.168.0.1:8080');

            await waitFor(() => getNodes().length === 0);
        });

    })
});
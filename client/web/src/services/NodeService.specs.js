import {updateNode, getNodes, clearNodes, removeNodeByAddress} from './NodeService'
import {toJS} from 'mobx'
describe('services/NodeService', () => {

    beforeEach(clearNodes);

    describe('getNodes()', () => {
        it('should get nodes', async () => {
            updateNode({address: 1});
            await waitFor(() => getNodes().length === 1);
            expect(getNodes()[0]).to.deep.equal({address: 1, nodeState: 'new'});
        })
    });

    describe('updateNode()', () => {
        it('should add nodes and trigger reactive', async () => {
            const spy = sinon.spy();
            autorun(() => {spy(getNodes().map(x => x))});
            expect(spy).to.have.been.calledOnce;
            expect(spy).to.have.been.calledWith([]);

            spy.reset();
            updateNode({address: 1});

            await waitFor(() => getNodes().length === 1);
            expect(spy).to.have.been.calledWith([{address: 1, nodeState: 'new'}]);

            spy.reset();
            updateNode({address: 2});
            await waitFor(() => getNodes().length === 2);
            expect(spy).to.have.been.calledWith([{address: 1, nodeState: 'new'}, {address: 2, nodeState: 'new'}]);
        })
    });

    describe('removeNodeByAddress()', () => {
        it('should remove a node given the correct address', async () => {
            updateNode({address:1});
            updateNode({address:2});
            updateNode({address:3});
            updateNode({address:4});

            await waitFor(() => getNodes().length === 4);
            expect(toJS(getNodes())).to.deep.equal([{address: 1, nodeState: 'new'}, {address:2, nodeState: 'new'}, {address:3, nodeState: 'new'}, {address: 4, nodeState: 'new'}]);
            removeNodeByAddress(3);
            expect(toJS(getNodes())).to.deep.equal([{address: 1, nodeState: 'new'}, {address:2, nodeState: 'new'}, {address: 3, nodeState: 'dead'}, {address: 4, nodeState: 'new'}]);
            removeNodeByAddress(1);
            expect(toJS(getNodes())).to.deep.equal([{address: 1, nodeState: 'dead'},{address:2, nodeState: 'new'}, {address: 3, nodeState: 'dead'}, {address: 4, nodeState: 'new'}]);
            removeNodeByAddress(4);
            expect(toJS(getNodes())).to.deep.equal([{address: 1, nodeState: 'dead'},{address:2, nodeState: 'new'}, {address: 3, nodeState: 'dead'}, {address: 4, nodeState: 'dead'}]);
            removeNodeByAddress(2);
            expect(toJS(getNodes())).to.deep.equal([{address: 1, nodeState: 'dead'},{address:2, nodeState: 'dead'}, {address: 3, nodeState: 'dead'}, {address: 4, nodeState: 'dead'}]);
        });

        it('should not remove any nodes if given an address of a non-existent node', async () => {
            updateNode({address: 1});

            await waitFor(() => getNodes().length === 1);
            removeNodeByAddress(5);
            expect(toJS(getNodes())).to.deep.equal([{address: 1, nodeState: 'new'}]);
        });

    })
});
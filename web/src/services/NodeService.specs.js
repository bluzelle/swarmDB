import {addNode, getNodes, clearNodes, removeNodeByAddress} from './NodeService'

describe('services/NodeService', () => {

    beforeEach(clearNodes);

    describe('getNodes()', () => {
        it('should get nodes', () => {
            addNode({address: 1});
            expect(getNodes()).to.have.length(1);
            expect(getNodes()[0]).to.deep.equal({address: 1});
        })
    });

    describe('addNode()', () => {
        it('should add nodes and trigger reactive', () => {
            const spy = sinon.spy();
            autorun(() => {spy(getNodes().map(x => x))});
            expect(spy).to.have.been.calledOnce;
            expect(spy).to.have.been.calledWith([]);

            spy.reset();
            addNode({address: 1});
            expect(spy.getCall(0).args[0])
            expect(spy).to.have.been.calledWith([{address: 1}]);

            spy.reset();
            addNode({address: 2});
            expect(spy).to.have.been.calledWith([{address: 1}, {address: 2}]);
        })
    });

    describe('removeNodeByAddress()', () => {
        it('should remove a node given the correct address', () => {
            addNode({address:1});
            addNode({address:2});
            addNode({address:3});
            addNode({address:4});
            expect(getNodes().toJS()).to.deep.equal([{address: 1}, {address:2}, {address:3}, {address: 4}]);
            removeNodeByAddress(3);
            expect(getNodes().toJS()).to.deep.equal([{address: 1}, {address:2}, {address: 4}]);
            removeNodeByAddress(1);
            expect(getNodes().toJS()).to.deep.equal([{address:2}, {address: 4}]);
            removeNodeByAddress(4);
            expect(getNodes().toJS()).to.deep.equal([{address:2}]);
            removeNodeByAddress(2);
            expect(getNodes()).to.have.length(0);
        });

        it('should not remove any nodes if given an address of a non-existent node', () => {
            addNode({address: 1});
            removeNodeByAddress(5);
            expect(getNodes().toJS()).to.deep.equal([{address: 1}]);
        });

        it('should be reactive', () => {
            addNode({address: 1});
            addNode({address: 2});
            addNode({address: 3});
            const spy = sinon.spy();

            autorun(() => spy(getNodes().map(x => x)));

            spy.reset();
            removeNodeByAddress(2);
            expect(spy).to.have.been.calledWith([{address: 1}, {address: 3}]);

            spy.reset();
            removeNodeByAddress(1);
            expect(spy).to.have.been.calledWith([{address: 3}]);

        });
    })
});
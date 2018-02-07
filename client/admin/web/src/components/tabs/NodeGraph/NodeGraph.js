import Node from './Node'
import {getNodes} from 'bluzelle-client-common/services/NodeService'
import NodeInfo from 'components/tabs/NodeInfo'

@observer
export default class NodeGraph extends Component {
    @observable selectedNodeAddress;

    selectNode(node) {
        this.selectedNodeAddress = node.address;
    }

    findNodeByAddress(address) {
        return getNodes().find(node => node.address === address);
    }

    addAnglesToNodes(nodes) {
        return nodes.map((node, idx) => {
            const angle = ((Math.PI * 2 / nodes.length) * idx);
            return {
                xAngle: Math.cos(angle),
                yAngle: Math.sin(angle),
                ...node
            }
        });
    };


    render() {
        const nodes = this.addAnglesToNodes(getNodes());
        const selectedNode = this.selectedNodeAddress ? this.findNodeByAddress(this.selectedNodeAddress) : undefined;

        return (
            <div style={{height: '100%', position: 'relative'}}>
                <svg style={{height: '100%', width: '100%'}} viewBox="0 0 200 200" xmlns="http://www.w3.org/2000/svg">
                    {nodes.map(node => <Node selected={selectedNode && node.address === selectedNode.address} onMouseOver={this.selectNode.bind(this, node)} key={`node-${node.address}`} node={node}/>)}
                </svg>
                <div style={styles.nodeCount}>
                    {nodes.length} Nodes
                </div>
                {selectedNode && <NodeInfo node={selectedNode} />}
            </div>
        )
    }
}

const styles = {
    nodeCount: {
        position: 'absolute',
        top: '20%',
        textAlign: 'center',
        fontWeight: 'bold',
        left: '50%',
        transform: 'translate(-50%, -50%)'
    }
};


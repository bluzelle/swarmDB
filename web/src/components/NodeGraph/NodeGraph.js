import  {getNodes} from 'services/NodeService'
import NodeGraphView from './NodeGraphView'

export default observer(() => <NodeGraphView nodes={getNodesWithLayout()} />);

const getNodesWithLayout = () => {
    const nodes = getNodes();
    return nodes.map((node, idx) => {
        const angle = ((Math.PI * 2 / nodes.length) * idx);
        return {
            xAngle: Math.cos(angle),
            yAngle: Math.sin(angle),
            ...node
        }
    });
};

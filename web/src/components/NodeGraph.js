import  {getNodes} from 'services/NodeService'

@observer
export default class NodeGraph extends Component {
    constructor() {
        super();
        this.state = {};
    }

    selectNode(node) {
        this.setState({selectedNodeAddress: node.address})
    }

    getLayout() {
        const nodes = getNodes();
        return nodes.map((node, idx) => {
            const angle = ((Math.PI * 2 / nodes.length) * idx);
            return {
                xAngle: Math.cos(angle),
                yAngle: Math.sin(angle),
                ...node
            }
        });
    }

    findNodeByAddress(address) {
        return getNodes().find(node => node.address === address);
    }

    render() {
        const {selectedNodeAddress} = this.state;
        const nodes = getNodes();
        const selectedNode = selectedNodeAddress ? this.findNodeByAddress(selectedNodeAddress) : undefined;

        return (
            <div style={{height: '100%', position: 'relative'}}>
                <svg style={{height: '100%', width: '100%'}} viewBox="0 0 200 200" xmlns="http://www.w3.org/2000/svg">
                    {this.getLayout().map(node => <Node selected={selectedNode && node.address === selectedNode.address} onMouseOver={this.selectNode.bind(this, node)} key={node.address} {...node}/>)}
                </svg>
                <div style={styles.nodeCount}>
                    {nodes.length} Nodes
                </div>
                {selectedNode &&
                <table style={styles.infoBox}>
                    <tbody>
                    <tr>
                        <th style={styles.tableCell}>Address</th>
                        <th style={styles.tableCell}>Messages</th>
                    </tr>
                    <tr>
                        <td style={styles.tableCell}>{selectedNode.address}</td>
                        <td style={styles.tableCell}>{selectedNode.messages}</td>
                    </tr>
                    </tbody>
                </table>
                }
            </div>
        )
    }
}

const styles = {
    tableCell: {
        padding: 5
    },
    infoBox: {
        width: 200,
        position: 'absolute',
        top: '50%',
        left: '50%',
        transform: 'translate(-50%, -50%)',
        border: '1px solid black'
    },
    nodeCount: {
        position: 'absolute',
        top: '20%',
        textAlign: 'center',
        fontWeight: 'bold',
        left: '50%',
        transform: 'translate(-50%, -50%)'
    }
};

const Node = ({address, messages, xAngle, yAngle, onMouseOver, selected}) => {
    const cx = 90 * xAngle + 100;
    const cy = 90 * yAngle + 100;

    return [
        selected && <circle fill='white' stroke="red" strokeWidth="1" key={`circle-border=${address}`} cx={cx} cy={cy} r="4"/>,
        <circle onMouseOver={onMouseOver} key={`circle-${address}`} cx={cx} cy={cy} r="3"/>
    ];
};
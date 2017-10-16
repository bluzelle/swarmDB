import StatusFormatter from 'components/tabs/StatusFormatter'
import {getNodeByAddress} from "../../services/NodeService";
import NodeMessagesBtn from './NodeMessagesBtn'

const NodeInfo = ({node: selectedNode}) => {
    const node = getNodeByAddress(selectedNode.address);
    return node ? (
        <table style={styles.infoBox}>
            <tbody>
            <tr>
                <th style={styles.tableCell}>Address</th>
                <td style={styles.tableCell}>{node.address}</td>
            </tr>
            <tr>
                <th style={styles.tableCell}>Messages</th>
                <td style={styles.tableCell}><NodeMessagesBtn address={node.address}/></td>
            </tr>
            <tr>
                <th style={styles.tableCell}>Status</th>
                <td style={styles.tableCell}><StatusFormatter value={node.nodeState}/></td>
            </tr>
            </tbody>
        </table>
    ) : <div></div>
};


export default observer(NodeInfo)

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
        borderWidth: 1,
        borderStyle: 'solid'
    }
};


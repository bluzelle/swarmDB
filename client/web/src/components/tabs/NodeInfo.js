import StatusFormatter from 'components/tabs/StatusFormatter'
import {getNodeByAddress} from "../../services/NodeService";
import NodeMessagesBtn from './NodeMessagesBtn'
import StorageDisplay from './StorageDisplay'
import PercentageDisplay from "./PercentageDisplay";

const NodeInfo = ({node: selectedNode}) => {
    const node = getNodeByAddress(selectedNode.address);
    return node ? (
        <table style={styles.infoBox}>
            <tbody>
            <InfoRow title="Address">{node.address}</InfoRow>
            <InfoRow title="Messages"><NodeMessagesBtn address={node.address}/></InfoRow>
            <InfoRow title="Status"><StatusFormatter value={node.nodeState}/></InfoRow>
            <InfoRow title="Available Space"><StorageDisplay size={node.available} /></InfoRow>
            <InfoRow title="Used"><StorageDisplay size={node.used} /> (<PercentageDisplay total={node.available} part={node.used}/>)</InfoRow>
            </tbody>
        </table>
    ) : <div></div>
};


export default observer(NodeInfo)

const InfoRow = ({title, children}) => (
    <tr>
        <th style={styles.tableCell}>{title}</th>
        <td style={styles.tableCell}>
            {children}
        </td>
    </tr>
);

const styles = {
    tableCell: {
        padding: 5,
        whiteSpace: 'nowrap'
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


import {getNodeByAddress, getNodes} from 'bluzelle-client-common/services/NodeService'
import RowSelectDataGrid from 'components/RowSelectDataGrid'
import {strafeObject} from 'src/Utils'
import NodeInfo from 'components/tabs/NodeInfo'
import StatusFormatter from 'components/tabs/StatusFormatter'
import NodeMessagesBtn from 'components/tabs/NodeMessagesBtn'
import ProgressBar from 'react-bootstrap/lib/ProgressBar'
import pipe from 'lodash/fp/pipe'
import getProp from 'lodash/fp/get'

@observer
export default class NodeListTabBody extends Component {

    constructor() {
        super();
        this.state = {};
    }

    rowGetter(idx) {
        const node = this.nodes[idx];
        return {actionAddress: node.address, ...node};
    }

    onRowSelect(node) {
        this.setState({selectedNode: node});
    }

    render() {
        this.nodes = getNodes().map(strafeObject);
        return (
            <Layout type="">
                <Flex>
                    <RowSelectDataGrid
                        selectByKey="address"
                        columns={columns}
                        rowGetter={this.rowGetter.bind(this)}
                        rowsCount={this.nodes.length}
                        minColumnWidth={80}
                        onRowSelect={this.onRowSelect.bind(this)}
                    />
                </Flex>
                <Fixed>
                    <div style={{padding: 10, width: 300}}>
                        {this.state.selectedNode && <NodeInfo node={this.state.selectedNode}/>}
                    </div>
                </Fixed>
            </Layout>
        )
    }
}


const ActionFormatter = ({value: address}) => (
    <NodeMessagesBtn address={address}/>
);

const StorageFormatter = ({dependentValues: node}) => {
    const percentUsed = Math.floor(node.used / node.available * 100);
    return (
        <ProgressBar key={node.address} style={{width: '100%', marginTop: 'auto', marginBottom: 'auto'}} now={percentUsed} label={`${percentUsed}%`}/>
    )
};


const AddressFormatter = pipe(
    getProp('value'),
    getNodeByAddress,
    node => `${node.address} ${node.isLeader ? '(L)' : ''}`
);

const columns = [{
    key: 'address',
    name: 'Address',
    resizable: true,
    width: 150,
    formatter: AddressFormatter
}, {
    key: 'nodeState',
    name: 'Status',
    resizable: true,
    width: 100,
    formatter: StatusFormatter
}, {
    key: n => console.log(n) || `storage-${n.address}`,
    name: 'Storage',
    resizable: true,
    width: 150,
    formatter: StorageFormatter,
    getRowMetaData: (row) => row
}, {
    key: 'actionAddress',
    name: 'Actions',
    resizable: true,
    formatter: ActionFormatter
}];

import {getNodes} from 'services/NodeService'
import RowSelectDataGrid from 'components/RowSelectDataGrid'
import {strafeObject} from 'src/Utils'
import NodeInfo from 'components/tabs/NodeInfo'
import StatusFormatter from 'components/tabs/StatusFormatter'
import NodeMessagesBtn from 'components/tabs/NodeMessagesBtn'

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
                <Flex>
                    <div style={{padding: 10}}>
                        {this.state.selectedNode && <NodeInfo node={this.state.selectedNode} />}
                    </div>
                </Flex>
            </Layout>
        )
    }
}



const ActionFormatter = ({value:address}) => (
    <NodeMessagesBtn address={address} />
);

const columns = [{
    key: 'address',
    name: 'Address',
    resizable: true,
    width: 150,
}, {
    key: 'nodeState',
    name: 'Status',
    resizable: true,
    width: 100,
    formatter: StatusFormatter,
}, {
    key: 'actionAddress',
    name: 'Actions',
    resizable: true,
    formatter: ActionFormatter
}];

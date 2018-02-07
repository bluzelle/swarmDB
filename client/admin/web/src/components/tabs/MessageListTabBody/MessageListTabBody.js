import {getMessages} from 'bluzelle-client-common/services/MessageService'
import DataGrid from 'components/DataGrid'
import clone from 'lodash/clone'
import defaults from 'lodash/defaults'

@observer
class MessageListTabBody extends Component {

    rowGetter(idx) {
        const message = this.messages[idx];
        return defaults({body: JSON.stringify(message.body)}, message)
    }

    addressFormatter({value}) {
        return this.props.address === value ? (
            <span style={{color: 'white'}}>{value}</span>
        ) : (
            <Link style={{color: '#aaa'}} to={`/message-list/filtered-by-address/${value}`}>{value}</Link>
        );
    }

    getColumns() {
        const filterMark = this.props.address ? '*' : '';
        return [{
            key: 'srcAddr',
            name: `${filterMark} Source Addr`,
            resizable: true,
            width: 150,
            formatter: this.addressFormatter.bind(this)
        }, {
            key: 'dstAddr',
            name: `${filterMark} Destination Addr`,
            resizable: true,
            width: 150,
            formatter: this.addressFormatter.bind(this)
        }, {
            key: 'body',
            name: 'Message',
            resizable: true
        }];
    }

    render() {
        const {address} = this.props;
        this.messages = address ? (
            getMessages().filter(m => [m.srcAddr, m.dstAddr].includes(address))
        ) : (
            getMessages()
        );

        return (
            <DataGrid
                columns={this.getColumns()}
                rowsCount={this.messages.length}
                rowGetter={this.rowGetter.bind(this)}
            />
        )
    }
}

export default withParams(MessageListTabBody)



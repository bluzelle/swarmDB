import {getLogEntries} from 'services/LogService'
import 'src/ReactPre16Support'
import DataGrid from 'components/DataGrid';
import pipe from 'lodash/fp/pipe'
import getProp from 'lodash/fp/get'

const LogTabBody = () => {
    const logEntries = getLogEntries();
    return (
        <DataGrid
            columns={columns}
            rowGetter={i => logEntries[i]}
            rowsCount={logEntries.length}
            minColumnWidth={120}
        />
    )
};

export default observer(LogTabBody)

const NodeFormatter = pipe(
    getProp('value.address'),
    v => v || ' '
);

const TimestampFormatter = ({value: timestamp}) => new Date(timestamp).toISOString();

const columns = [{
    key: 'level',
    name: 'Level',
    resizable: true,
    width: 100
},{
    key: 'timestamp',
    name: 'Timestamp',
    resizable: true,
    width: 200,
    formatter: TimestampFormatter
}, {
    key: 'message',
    name: 'Message',
    resizable: true
}, {
    key: 'node',
    name: 'Node',
    resizable: true,
    width: 150,
    formatter: NodeFormatter

}];


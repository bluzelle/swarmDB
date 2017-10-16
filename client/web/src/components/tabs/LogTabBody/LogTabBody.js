import {getLogEntries} from 'services/LogService'
import 'src/ReactPre16Support'
import DataGrid from 'components/DataGrid';


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


const columns = [{
    key: 'timer_no',
    name: 'Timer loop #',
    resizable: true,
    width: 100
}, {
    key: 'entry_no',
    name: 'Entry #',
    resizable: true,
    width: 100
}, {
    key: 'timestamp',
    name: 'Timestamp',
    resizable: true,
    width: 150
}, {
    key: 'message',
    name: 'Message',
    resizable: true
}];


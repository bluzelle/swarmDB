import DataGrid from 'components/DataGrid';
import getProp from 'lodash/get'
import './row-select-grid.css'


export default class RowSelectDataGrid extends Component {

    constructor() {
        super();
        this.state = {}
    }

    ensureSelectedRowStillExists() {
        const {selectByKey} = this.props;
        !this.state.selectedRow ||
        this.props.rows.some(row => row[selectByKey] === this.state.selectedRow[selectByKey]) ||
        this.setState({selectedRow: undefined});
    }

    componentWillUpdate() {
//        this.ensureSelectedRowStillExists();
    }

    selectRow(idx) {
        const rowData = this.props.rowGetter(idx);
        this.setState({selectedRow: rowData});
        this.props.onRowSelect && this.props.onRowSelect(rowData);
    }

    render() {
        const {selectByKey, rowGetter, rows, rowsCount, ...props} = this.props;
        const {selectedRow} = this.state;
        return (
            <DataGrid
                {...props}
                rowsCount ={rowsCount}
                rowGetter={rowGetter}
                rowSelection={{
                    selectBy: {keys: {rowKey: selectByKey, values: [getProp(selectedRow, selectByKey)]}},
                    showCheckbox: false
                }}
                onRowClick={this.selectRow.bind(this)}
            />
        )
    }
}

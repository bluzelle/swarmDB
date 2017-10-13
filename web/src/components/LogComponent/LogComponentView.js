import 'src/ReactPre16Support'

const ReactDataGrid = require('react-data-grid');

export default class LogComponentView extends Component {

    constructor() {
        super();
        this.state = {
            gridHeight: 100
        }
        this.resizeGrid = () => this.setState({gridHeight: this.wrapper.clientHeight})
    }

    componentDidMount() {
        window.addEventListener('resize', this.resizeGrid);
        this.resizeGrid();
    }

    componentWillUnmount() {
        window.removeEventListener('resize', this.resizeGrid);
    }


    render() {
        const {logEntries} = this.props;
        const {gridHeight} = this.state;
        return (
            <div ref={r => this.wrapper = r} style={{height: '100%'}}>
                <ReactDataGrid
                    columns={columns}
                    rowGetter={i => logEntries[i]}
                    rowsCount={logEntries.length}
                    minHeight={gridHeight}
                    minColumnWidth={120}
                />
            </div>
        )
    }
}

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


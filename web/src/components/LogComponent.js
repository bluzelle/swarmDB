import 'src/ReactPre16Support'
import {getLogEntries} from 'services/LogService'

const ReactDataGrid = require('react-data-grid');

@observer
export default class LogComponent extends Component {

    constructor() {
        super();
        this.state = {
            gridHeight: 100
        }
        this.resizeGrid = () => this.setState({gridHeight: this.wrapper.clientHeight})
    }

    columns() {
        return [{
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
            width: 250
        }, {
            key: 'message',
            name: 'Message',
            resizable: true
        }]
    }

    componentDidMount() {
        window.addEventListener('resize', this.resizeGrid);
        this.resizeGrid();
    }

    componentWillUnmount() {
        window.removeEventListener('resize', this.resizeGrid);
    }


    render() {
        const {gridHeight} = this.state;
        return (
            <div ref={r => this.wrapper = r} style={{height: '100%'}}>
                <ReactDataGrid
                    columns={this.columns()}
                    rowGetter={i => getLogEntries()[i]}
                    rowsCount={getLogEntries().length}
                    minHeight={gridHeight}
                    minColumnWidth={120}
                />
            </div>
        )
    }
}
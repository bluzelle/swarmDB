import Row from './Row'

@observer
export default class EmulatorTabBody extends Component {


    componentWillMount() {
        this.emulator = electron.remote.require('./emulator/Emulator');
        this.setState({maxNodes: this.emulator.getMaxNodes()});
    }

    setMaxNodes(maxNodes) {
        this.emulator.setMaxNodes(maxNodes);
        this.setState({maxNodes: maxNodes});
    }

    render() {
        const {maxNodes} = this.state;
        return (
                <Layout type="column" style={{padding: 20}}>
                    <Row id="max-nodes-setting" label="Max Nodes" setFn={this.setMaxNodes.bind(this)} type="number" value={maxNodes}/>
                </Layout>
        )
    }
}



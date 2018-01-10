import InputRow from './InputRow'
import ToggleRow from './ToggleRow'

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
                <InputRow id="max-nodes-setting" label="Max Nodes" setFn={this.setMaxNodes.bind(this)} type="number" value={maxNodes}/>
                <ToggleRow id="toggle-random" label="Random behavior" setFn={this.emulator.behaveRandomly} value={this.emulator.isRandom()} />
            </Layout>
        )
    }
}




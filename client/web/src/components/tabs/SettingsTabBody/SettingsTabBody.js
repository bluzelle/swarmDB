import {settings, setMaxNodes, setMinNodes} from 'services/SettingsService'
import Row from './Row'

@observer
export default class SettingsTabBody extends Component {

    render() {
        return (
                <Layout type="column" style={{padding: 20}}>
                    <Row id="max-nodes-setting" label="Max Nodes" setFn={setMaxNodes} type="number" value={settings.maxNodes}/>
                    <Row id="min-nodes-setting" label="Min Nodes" setFn={setMinNodes} type="number" value={settings.minNodes}/>
                </Layout>
        )
    }
}



import Tabs from './MainTabs'
import NodeGraph from 'components/NodeGraph'
import LogComponent from 'components/LogComponent'

export default class Main extends Component {
    render() {
        return (
            <Layout type="column">
                <Fixed>
                    <div style={{height: 50}}>
                        <Tabs/>
                    </div>
                </Fixed>
                <Flex style={{overflow: 'auto'}}>
                    <Switch>
                        <Route path="/node-graph" component={NodeGraph}/>
                        <Route component={LogComponent}/>
                    </Switch>
                </Flex>
            </Layout>
        )
    }
}
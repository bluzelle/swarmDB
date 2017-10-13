import Tabs from './MainTabs'
import NodeGraph from 'components/NodeGraph'
import LogComponent from 'components/LogComponent'
import logo from './logo-color.png'

export default class Main extends Component {
    render() {
        return (
            <Layout type="column">
                <Fixed>
                    <div style={{height: 50, padding: 2}}>
                        <img src={logo} />
                    </div>
                    <div style={{height: 6}}/>
                </Fixed>
                <Fixed>
                    <div style={{height: 40}}>
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
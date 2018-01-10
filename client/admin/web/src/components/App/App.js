import {HashRouter, Route} from 'react-router-dom'
import './bootstrap/css/darkly.css'
import './style.css'
import {socketState} from 'services/CommunicationService'
import Main from 'components/Main'
import DaemonSelector from 'components/DaemonSelector'
import DevTools from 'mobx-react-devtools';
import {getNodes} from 'services/NodeService'

const App = () => {
    return (
        <div style={{height: '100%'}}>
            {/dev-tools/.test(window.location.href) && <DevTools/>}
            <HashRouter>
                <Route component={getNodes().length ? Main : DaemonSelector} />
            </HashRouter>
        </div>
    )
};

export default observer(App)


import {HashRouter, Route} from 'react-router-dom'
import './bootstrap/css/darkly.css'
import './style.css'
import Main from 'components/Main'
import DaemonSelector from 'bluzelle-client-common/components/DaemonSelector'
import DevTools from 'mobx-react-devtools';
import {getNodes} from 'bluzelle-client-common/services/NodeService'

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


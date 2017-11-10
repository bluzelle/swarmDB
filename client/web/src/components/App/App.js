import {HashRouter, Route} from 'react-router-dom'
import './bootstrap/css/darkly.css'
import './style.css'
import {socketState} from 'services/CommunicationService'
import Main from 'components/Main'
import DaemonSelector from 'components/DaemonSelector'
import DevTools from 'mobx-react-devtools';

const App = () => {
    const component = socketState.get() === 'open' ? Main : DaemonSelector;
    return (
        <div style={{height: '100%'}}>
            <DevTools/>
            <HashRouter>
                <Route component={component}/>
            </HashRouter>
        </div>
    )
};

export default observer(App)


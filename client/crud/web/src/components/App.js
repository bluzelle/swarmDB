import {HashRouter, Route} from 'react-router-dom'
import {Main} from 'components/Main'
import {execute, enableExecutionForChildren} from "../services/CommandQueueService";
import DevTools from 'mobx-react-devtools';
import {getNodes} from 'bluzelle-client-common/services/NodeService'
import 'bluzelle-client-common/services/CommunicationService';
import DaemonSelector from 'bluzelle-client-common/components/DaemonSelector'

// Debugging
// import {configureDevtool} from 'mobx-react-devtools';
// configureDevtool({logEnabled: true});


import {sendToNodes} from "bluzelle-client-common/services/CommunicationService";
import {when} from 'mobx';

const socketOpen = () => getNodes().some(node => node.socketState === 'open');
const getKeyList = () => sendToNodes('requestKeyList');

when(socketOpen, getKeyList);


@observer
@enableExecutionForChildren
export class App extends Component {
    getChildContext() {
        return {execute};
    }

    render() {
        return (
            <div style={{height: '100%'}}>
                {/dev-tools/.test(window.location.href) && <DevTools/>}
                <HashRouter>
                    <Route component={getNodes().length ? Main : DaemonSelector} />
                </HashRouter>
            </div>
        );
    }
};

import PropTypes from 'prop-types';
import {HashRouter, Route} from 'react-router-dom'
import {Main} from 'components/Main'
import {execute, setExecuteContext} from "../services/CommandQueueService";
import DevTools from 'mobx-react-devtools';

@setExecuteContext
export class App extends Component {
    getChildContext() {
        return {execute};
    }

    render() {
        return (
            <div style={{height: '100%'}}>
                <DevTools/>
                <HashRouter>
                    <Route component={Main} />
                </HashRouter>
            </div>
        );
    }
};
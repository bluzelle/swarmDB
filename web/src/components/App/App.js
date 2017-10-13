import {HashRouter, Route} from 'react-router-dom'
import 'bootstrap/dist/css/bootstrap.css'

import Main from 'components/Main'

const App = () => (
    <HashRouter>
        <Route component={Main}/>
    </HashRouter>
);

export default App


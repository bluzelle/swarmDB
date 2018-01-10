import ReactDom from 'react-dom'
import {App} from "./components/App"
import 'react-reflex/styles.css'


ReactDom.render(<App />, document.querySelector('#app-container'));

// Load files in /services
// const testsContext = require.context('./services', true, /Service.js$/);
// testsContext.keys().forEach(testsContext);

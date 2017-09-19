import ReactDom from 'react-dom'
import App from 'components/App'


ReactDom.render(<App />, document.querySelector('#app-container'), () => setTimeout(() => Session.set('ready', true), 2000));

// Load files in /services
const testsContext = require.context('./services', true, /Service.js$/);
testsContext.keys().forEach(testsContext);

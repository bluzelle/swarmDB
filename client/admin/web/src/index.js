import ReactDom from 'react-dom'
import App from 'components/App'
import './requestNodeInfo';


ReactDom.render(<App />, document.querySelector('#app-container'));

// Load files in /services
const testsContext = require.context('bluzelle-client-common/services', true, /Service.js$/);
testsContext.keys().forEach(testsContext);

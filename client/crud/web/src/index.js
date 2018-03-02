import ReactDom from 'react-dom'
import {App} from "./components/App"
import 'react-reflex/styles.css'
import {addHooks} from "react-functional-test";
import './requestNodeInfo';

const root = ReactDom.render(<App />, document.querySelector('#app-container'));



if(window.location.href.includes('test')) {

    const components = {};

    const context = require.context('./components', true, /\.js/);
    context.keys().forEach(path =>

        // Do not include test files
        path.includes('specs') ||

        Object.assign(components, context(path)));


    addHooks(root, components);

}
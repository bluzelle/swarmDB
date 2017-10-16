import 'babel-polyfill'
import chai, { expect } from 'chai';
import Enzyme, { mount, shallow, render } from 'enzyme';
import sinon from 'sinon';

import chaiEnzyme from 'chai-enzyme'
import sinonChai from 'sinon-chai'

import Adapter from 'enzyme-adapter-react-16';
import {Maybe} from 'monet'

Enzyme.configure({ adapter: new Adapter() });
chai.use(chaiEnzyme());
chai.use(sinonChai);


global.expect = expect;
global.mount = mount;
global.shallow = shallow;
global.render = render;
global.sinon = sinon;



const testsContext = require.context('.', true, /\.specs\.js$/);
testsContext.keys().forEach(testsContext);

const  componentsContext = require.context('./components', true, /\/.js$/);
componentsContext.keys().forEach(componentsContext);

const servicesContext = require.context('./services', true, /\/.js$/);
servicesContext.keys().forEach(servicesContext);


//Mock WebSocket
global.WebSocket = function() {
    return {}
};

beforeEach(() => Maybe.fromNull(document.querySelector('#show'))
    .map(el => el.innerHTML = '')
);

global.shallowShow = (...args) => {
    const out = shallow(...args);
    document.querySelector('#show').innerHTML = out.html();
    return out;
};

global.renderShow = (...args) => {
    const out = render(...args);
    document.querySelector('#show').innerHTML = out.html();
    return out;
};



//window.location.host = window.location.host || (window.location.host = 'localhost');
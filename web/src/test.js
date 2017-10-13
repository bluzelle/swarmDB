import chai, { expect } from 'chai';
import Enzyme, { mount, shallow, render } from 'enzyme';
import sinon from 'sinon';

import chaiEnzyme from 'chai-enzyme'

import Adapter from 'enzyme-adapter-react-16';


Enzyme.configure({ adapter: new Adapter() });
chai.use(chaiEnzyme());


global.expect = expect;
global.mount = mount;
global.shallow = shallow;
global.render = render;
global.sinon = sinon;

const testsContext = require.context('.', true, /\.specs\.js$/);
testsContext.keys().forEach(testsContext);
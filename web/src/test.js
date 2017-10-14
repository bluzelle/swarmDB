import chai, { expect } from 'chai';
import Enzyme, { mount, shallow, render } from 'enzyme';
import sinon from 'sinon';

import chaiEnzyme from 'chai-enzyme'
import sinonChai from 'sinon-chai'

import Adapter from 'enzyme-adapter-react-16';


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
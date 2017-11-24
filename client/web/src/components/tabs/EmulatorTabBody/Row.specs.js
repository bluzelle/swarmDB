import Row from './Row'

describe('components/SettingsTabBody/Row', () => {
    let wrapper, setSpy = sinon.spy();

    beforeEach(() => {
        wrapper = mount(<Row value="something" setFn={setSpy}/>);
    });

    afterEach(() => {
        wrapper.unmount();
    });

    it('should render a "Edit" button first', () => {
        const $el = $j(wrapper.html());
        expect($el.find('button:contains(Edit)')).to.have.length(1);
    });

    it('should render the value as text', () => {
        const $el = $j(wrapper.html());
        expect($el.find('div>:contains(something)')).to.have.length(1);
    });

    it('should render a input with the value when the "edit" button is clicked', () => {
        wrapper.find('button').simulate('click');
        const $el = $j(wrapper.html());
        expect($el.find('input').val()).to.equal('something');
    });

    it('should send the new value to the "setFn"', () => {
        wrapper.find('button').simulate('click');
        wrapper.find('input').getDOMNode().value = 'somethingElse';
        wrapper.find('button').at(0).simulate('click');
        expect(setSpy).to.have.been.calledWith('somethingElse');
    });


});
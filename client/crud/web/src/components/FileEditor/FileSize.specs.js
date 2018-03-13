import {FileSize} from "./FileSize";

describe.only('File Size', () => {

    it('should display bytes', () => {

        const wrapper = mount(<FileSize numBytes={634} />);

        expect(wrapper.text()).to.equal('634 bytes');

    });

    it('should display KB', () => {

        const wrapper = mount(<FileSize numBytes={13634} />);

        expect(wrapper.text()).to.equal('13 KB');

    });

    it('should display MB', () => {

        const wrapper = mount(<FileSize numBytes={23513634} />);

        expect(wrapper.text()).to.equal('23 MB');

    });

    it('should display GB', () => {

        const wrapper = mount(<FileSize numBytes={99123513634} />);

        expect(wrapper.text()).to.equal('99 GB');

    });

});
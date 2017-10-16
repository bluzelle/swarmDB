import SettingsTabBody from './SettingsTabBody'

describe('tabs/SettingsTabBody', () => {
    it('should mount without errors', () => {
        const wrapper = mount(<SettingsTabBody/>);
        wrapper.unmount();
    });
});
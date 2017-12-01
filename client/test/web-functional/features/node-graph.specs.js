
describe('Node graph tab', () => {
    require('../getBaseElement')('body');

    beforeEach(() => {
        browser.waitForExist('=Node Graph', 2000);
        browser.click('=Node Graph');
    });

    describe('individual nodes', () => {
            it(`should display specs when mouseover on node`, () => {
                const node = Emulator.getNodes()[0];
                checkInfoTable(node.address);
            });
        });
    });

const checkInfoTable = (address) => {
    const idAddress = address.replace(':', '-');
    browser.waitForExist(`g[data-test="node-${idAddress}"]`, 2000);
    browser.moveToObject(`g[data-test="node-${idAddress}"]`);
    browser.waitForExist(`td=${address}`, 2000);
};
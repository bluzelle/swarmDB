const _ = require('lodash');
import {addNode, updateNode} from "../CommunicationService";

describe('Node graph tab', () => {
    require('../getBaseElement')('body');

    beforeEach(() => {
        browser.waitForExist('=Node Graph', 2000);
        browser.click('=Node Graph');
    });

    describe('individual nodes', () => {
        _.each({green: 'alive', red: 'dead', blue: 'new'}, (state, color) => {
            it(`should display specs when mouseover on ${color} node`, () => {
                const nodeInfo = addNode({nodeState: state});
                updateNode(nodeInfo.address, {nodeState: state});
                checkInfoTable(nodeInfo.address, state);
                checkInfoTable(nodeInfo.address, nodeInfo.address);
            });
        });
    });
});

const checkInfoTable = (address, value) => {
    browser.waitForExist(`g#node-${address}`, 2000);
    browser.moveToObject(`g#node-${address}`);
    browser.waitForExist(`td=${value}`, 2000);
}
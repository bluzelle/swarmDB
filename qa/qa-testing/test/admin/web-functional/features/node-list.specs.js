const {clickTab} = require('../utils');
const _ = require('lodash');

describe('Node List tab', () => {

    describe('Table Headers', () => {

        beforeEach(() => clickTab('Node List'));

        it('should contain table headers', () => {
            ['Address', 'Status', 'Actions'].forEach(text => {
                browser.waitForExist(`div.widget-HeaderCell__value*=${text}`);
            });
        });
    });

    describe('Table Rows', () => {
        const NUM_OF_NODES = 2;

        beforeEach(() => {
            clickTab('Node List');
            emulator.setMaxNodes(NUM_OF_NODES);
        });

        it('should show all nodes', () => {
            browser.waitUntil(() => {
                return browser.elements('div.react-grid-Canvas>div>div').value.length === NUM_OF_NODES
            }, 5000, 'expected nodes to equal number set by emulator');
        });

        it('should show all nodes to be new then alive', () => {
            browser.waitUntil(() =>
                browser.elements('div.react-grid-Canvas>div>div').value.length === NUM_OF_NODES
            );

            ['new', 'alive'].forEach( status => {
                browser.elements('div.react-grid-Canvas>div>div').value.forEach(el => {
                    el.waitForExist(`div=${status}`);
                });
            });
        });
    });
});

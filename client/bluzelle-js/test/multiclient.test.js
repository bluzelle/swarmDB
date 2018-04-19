const reset = require('./reset');
const assert = require('assert');

const path = require('path');

const api1 = require('../api');


// This enables us to have two copies of the library with seperate state

delete require.cache[path.resolve(__dirname + '/../communication.js')];
delete require.cache[path.resolve(__dirname + '/../api.js')];

const api2 = require('../api');


// Run if testing in node, otherwise skip
(typeof window === 'undefined' ? describe : describe.skip)('multi-client bluzelle api', () => {


    describe('two clients with different UUID\'s interacting with the same key', () => {

        beforeEach(reset);

        beforeEach(async () => {
            await api1.connect('ws://localhost:8100', '4982e0b0-0b2f-4c3a-b39f-26878e2ac814');
            api1.setup();

            await api2.connect('ws://localhost:8100', '45a61b71-7f20-4355-99d4-3780bad17d84');
            api2.setup();
        });

        afterEach(() => {
            api1.disconnect();
            api2.disconnect();
        });

        it('api1 should be able to ping the connection', () =>
            api1.ping());

        it('api2 should be able to ping the connection', () =>
            api2.ping());

        it('api1 should be able to write to database', async () => {
            await api1.update('myKey', 123);
            assert(await api1.read('myKey') === 123);
        });

        it('api2 should be able to write to database', async () => {
            await api2.update('myKey', 345);
            assert(await api2.read('myKey') === 345);
        });

        describe('number fields', async () => {

            beforeEach(async () => {
                await api1.update('myKey', 123);
                await api2.update('myKey', 345);
            });

            it('should be able to read with no collision', async () => {
                assert(await api1.read('myKey') === 123);
                assert(await api2.read('myKey') === 345);
            });

            it('should be able to update from one and not affect the other', async () => {
                await api1.update('myKey', 999);

                assert(await api2.read('myKey') === 345);
            });

            it('should be able to delete from one and not affect the other', async () => {
                await api1.remove('myKey');

                assert(await api2.read('myKey') === 345);
            });

        });

        describe('text fields', async () => {

            beforeEach(async () => {
                await api1.update('myKey', 'hello world');
                await api2.update('myKey', 'good morning');
            });

            it('should be able to read with no collision', async () => {
                assert(await api1.read('myKey') === 'hello world');
                assert(await api2.read('myKey') === 'good morning');

            });

            it('should be able to update from one and not affect the other', async () => {
                await api1.update('myKey', 'changed value');

                assert(await api2.read('myKey') === 'good morning');
            });

            it('should be able to delete from one and not affect the other', async () => {
                await api1.remove('myKey');

                assert(await api2.read('myKey') === 'good morning');
            });

        });

        describe('object fields', async () => {

            beforeEach(async () => {
                await api1.update('myKey', {a: 5});
                await api2.update('myKey', {b: 9});
            });

            it('should be able to read with no collision', async () => {
                assert((await api1.read('myKey')).a === 5);
                assert((await api2.read('myKey')).b === 9);

            });

            it('should be able to update from one and not affect the other', async () => {
                await api1.update('myKey', {a: 500});

                assert((await api2.read('myKey')).b === 9);
            });

            it('should be able to delete from one and not affect the other', async () => {
                await api1.remove('myKey');

                assert((await api2.read('myKey')).b === 9);
            });

        });

        describe('attempting to access keys of another client', () => {

            beforeEach(async () => {
                await api1.update('onlyInOne', 'something');
            });

            it('should only be able to access keys in its database', async () => {
                assert(await api1.has('onlyInOne'));
                assert(!await api2.has('onlyInOne'));
            });

            it('should throw an error when trying to read a key not in its database', done => {
                api2.read('onlyInOne').catch(() => done());
            });

            it('should throw an error when trying to update a key not in its database', done => {
                api2.remove('onlyInOne').catch(() => done());
            });

            it('should throw an error when trying to delete a key not in its database', done => {
                api2.remove('onlyInOne').catch(() => done());
            });

        });
    });

    describe('two clients with the same UUID\'s interacting with the same key', () => {

        beforeEach(reset);

        beforeEach(async () => {
            await api1.connect('ws://localhost:8100', '4982e0b0-0b2f-4c3a-b39f-26878e2ac814');
            api1.setup();

            await api2.connect('ws://localhost:8100', '4982e0b0-0b2f-4c3a-b39f-26878e2ac814');
            api2.setup();
        });

        afterEach(() => {
            api1.disconnect();
            api2.disconnect();
        });

        it('api1 should be able to ping the connection', () =>
            api1.ping());

        it('api2 should be able to ping the connection', () =>
            api2.ping());

        it('api1 should be able to write to database', async () => {
            await api1.update('myKey', 123);
            assert(await api1.read('myKey') === 123);
        });

        it('api2 should be able to write to database', async () => {
            await api2.update('myKey', 345);
            assert(await api2.read('myKey') === 345);
        });

        describe('api1\'s key should be mutated by api2\'s call', async () => {

            describe('creating, updating, and then reading', () => {

                describe('number fields', () => {

                    beforeEach(async () => {
                        await api1.update('myKey', 123);
                        await api2.update('myKey', 345);
                    });

                    it('value should be equal to last call', async () => {
                        assert(await api1.read('myKey') !== 123);
                        assert(await api1.read('myKey') === 345);
                    });
                });

                describe('text fields', () => {

                    beforeEach(async () => {
                        await api1.update('myKey', 'hello world');
                        await api2.update('myKey', 'goodbye world');
                    });

                    it('value should be equal to last call', async () => {
                        assert(await api1.read('myKey') !== 'hello world');
                        assert(await api1.read('myKey') === 'goodbye world');
                    });
                });

                describe('object fields', () => {

                    beforeEach(async () => {
                        await api1.update('myKey', { a : 5 });
                        await api2.update('myKey', { a : 100});
                    });

                    it('value should be equal to last call', async () => {
                        assert((await api1.read('myKey')).a !== 5 );
                        assert((await api1.read('myKey')).a === 100);
                    });
                });

            });

            describe('creating, deleting, and then reading', () => {

                describe('number field', () => {

                    beforeEach(async () => {
                        await api1.update('myKey', 123);
                        await api2.remove('myKey');
                    });

                    it('throw error when attempting to read', done => {
                        api1.read('myKey').catch(() => done());
                    });

                });

                describe('text field', () => {

                    beforeEach(async () => {
                        await api1.update('myKey', 'hello world');
                        await api2.remove('myKey');
                    });

                    it('throw error when attempting to read', done => {
                        api1.read('myKey').catch(() => done());
                    });

                });

                describe('object field', () => {

                    beforeEach(async () => {
                        await api1.update('myKey', { a : 5 });
                        await api2.remove('myKey');
                    });

                    it('throw error when attempting to read', done => {
                        api1.read('myKey').catch(() => done());
                    });

                });
            });
        });
    });
});

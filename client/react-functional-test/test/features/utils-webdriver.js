import {start, close} from '../testServer';
import {expect} from 'chai';

import {executeAsyncWithError} from "../../react-webdriver";


describe('Webdriver utils', () => {

    before(() => {
        start(8200);
    });

    beforeEach(() => {
        browser.url('http://localhost:8200');
    });

    after(() => {
        close();
    });


    // The application we are testing must import our library and call
    // addHooks().

    it('should find hooks', () => {

        browser.execute(() => {

            if (!window.hooks) {
                throw new Error('addHooks() not being called.');
            }

        });

    });


    it('should not expose app data with flag', () => {
        browser.url('http://localhost:8200/?noHooks');

        browser.execute(() => {

            if (window.hooks) {
                throw new Error('addHooks() is being called.');
            }

        });

    });


    describe('assert and browser.execute', () => {

        // There are two main ways to verify conditions in the browser:
        // browser.execute and browser.executeAsync. The first is used
        // for checking for conditions at the time of execution and the
        // latter for asynchronous ones.

        // The function inside of browser.execute is called in the
        // browser context. We have a function assert(f) that can be
        // used to throw errors.

        it('should pass a valid assertion', () => {

            browser.execute(() => assert(() => true));

        });


        it('should pipe assertion errors to chimp', () => {

            const errorFunction = () =>
                browser.execute(() => assert(() => false));

            expect(errorFunction).to.throw();

        });


        it('should work with the browser\'s global namespace', () => {

            browser.execute(() => myGlobal = 123);

            browser.execute(() => assert(() => myGlobal === 123));

            browser.execute(() => assert(() => 'myGlobal' in window));

            expect(typeof myGlobal).to.equal('undefined');

        });


        it('should be able to pass serializable arguments to the browser', () => {

            browser.execute(
                (x, y) => assert(() => x === 123 && y === 'string'),
                123,
                'string'
            );

        });


        it('should not be able to pass non-serializable arguments to the browser', () => {

            browser.execute(
                f => assert(() => typeof f !== 'function'),
                () => 123
            );

        });


        it('should not include the chimp scope', () => {

            const myVariable = 123;

            browser.execute(() => assert(() => typeof myVariable === 'undefined'));

        });

    });


    describe('browser.executeAsync() and executeAsyncWithError()', () => {

        // excuteAsyncWithError() is a wrapper for browser.executeAsync() that
        // can pipe asynchronous errors back to chimp.


        it('should pass a valid async', () => {

            browser.executeAsync(done => done());
            executeAsyncWithError(browser, done => done());

        });


        it('should fail if done() is not called', () => {

            browser.timeouts('script', 200);

            expect(() =>
                browser.executeAsync(done => {})
            ).to.throw();

            expect(() =>
                executeAsyncWithError(browser, done => {})
            ).to.throw();

        });


        it('should pass args the same', () => {

            browser.executeAsync((x, done) => {
                assert(() => x === 5);
                done();
            }, 5);

            executeAsyncWithError(browser, (x, done) => {
                assert(() => x === 5);
                done();
            }, 5);

        });


        it('should not pipe asynchronous errors to chimp with browser.executeAsync()', () => {

            browser.timeouts('script', 200);

            expect(
                () => browser.executeAsync(done => {
                    throw new Error('our error');
                })
            ).to.throw('our error');

            expect(
                () => browser.executeAsync(done =>
                    setTimeout(() => {
                        throw new Error('our error');
                    }, 10))

            // 'script timeout' here means that we are not receiving our intended error
            ).to.throw('script timeout');

        });


        it('should pipe asynchronous errors correctly with executeAsyncWithError', () => {

            expect(
                () => executeAsyncWithError(browser, done =>
                    throwAsync(new Error('our error'), done))
            ).to.throw('our error');

            expect(
                () => executeAsyncWithError(browser, done =>
                    setTimeout(() =>
                        throwAsync(new Error('our error'), done),
                        10))
            ).to.throw('our error');

        });

    });


    describe('waitFor()', () => {

        // The function waitFor() is a utility that polls until some synchronous
        // function does not throw.


        it('should pass basic condition with no error', () => {

            executeAsyncWithError(browser, done =>
                waitFor(() => {}, done).then(done));

        });


        it('should fail after specified time', () => {

            const failure = () => executeAsyncWithError(browser, done =>
                waitFor(() => assert(() => false), done, 100));

           expect(failure).to.throw();

        });


        it('should eventually pass', () => {

            executeAsyncWithError(browser, done => {

                let x = false;

                waitFor(() => assert(() => x), done, 200).then(done);

                setTimeout(() => x = true, 50);

            });
        });

    });

});
export const tick = observable(0);

export const onTick = fn => autorun(() => {
    tick.get();
    fn();
});

(function ticker() {
    tick.set(tick.get() + 1);
    setTimeout(ticker, 1000);
}());
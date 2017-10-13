const logEntries = observable([]);

export const getLogEntries = () => logEntries.toJS();

// Fake stuff below this
const entries = [1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16].forEach(n => {
    setTimeout(() => {
        logEntries.push({
            'timer_no': n,
            'entry_no': n,
            'timestamp': new Date().toString(),
            'message' : `message-${n}`
        })
    }, n * 250);
});



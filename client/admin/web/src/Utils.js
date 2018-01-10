// touch all of the getters on an object.  This is used to make properties reactive in mobX
export const strafeObject = (obj) => {
    Object.keys(obj).map(key => obj[key]);
    return obj;
};

export const transactionBundler  = (name, fn) => {
    let calls = [];

    const executeFunctions = action(name, () => {
        // TODO: Bring calls in so I can make this async in the future
        const myCalls = calls;
        calls = [];
        myCalls.forEach(args => fn.apply(null, args));
    });

    return (...args) => {
        calls.length || setTimeout(executeFunctions, 100);
        calls.push(args);
    }
};



const values = observable({});

export default global.Session = {
    set: (k, v) => {
        extendObservable(values, {[k]: v});
    },
    get: (k) => {
        values[k] !== undefined || extendObservable(values, {[k]: undefined});
        return values[k];
    }
}



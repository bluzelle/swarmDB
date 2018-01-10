import {isObservableArray} from 'mobx';
import {isPlainObject, mapValues} from 'lodash';


export const get = (obj, propName) =>
    isObservableArray(obj) ? obj[propName] : obj.get(propName);

export const del = (obj, propName) =>
    isObservableArray(obj) ? obj.splice(propName, 1) : obj.delete(propName);


// Default mobX observable.map({}) functionality wraps nested objects as
// observable objects. This function wraps them as observable maps.

export const observableMapRecursive = obj =>
    isPlainObject(obj) ? observable.map(mapValues(obj, observableMapRecursive)) :
        Array.isArray(obj) ? observable.array(obj.map(observableMapRecursive)) : obj;
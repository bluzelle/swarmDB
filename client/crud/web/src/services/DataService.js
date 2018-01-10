import {objectToKeyData} from '../components/JSONEditor/JSONEditor';
import {textToKeyData} from "../components/PlainTextEditor";
import {observableMapRecursive} from "../util/mobXUtils";

const data = observableMapRecursive({

    key1: objectToKeyData({
        array: [1, 2, 3, 4]
    }),

    anotherKey: objectToKeyData({
        fieldA: 1.23,
        fieldB: 4.56,
        bool: true,
        crazyObject: {
            "true": false
        }
    }),

    complexObject: objectToKeyData({
        arrays: [1, 2, [{field: "feild"}, []], 3, ["apples", ["and", ["oranges"]]]]
    }),

    someText: textToKeyData("Hello world, this is some plain text.")
});

export const getSwarmData = () => data;
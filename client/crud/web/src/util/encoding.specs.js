import {strToByteArray, byteArrayToStr, strToArray, arrayToStr} from "./encoding";

describe('Text encoding', () => {

    it('Should convert a string to a byte array, back again.', () => {

        const str = "sample string";

        expect(byteArrayToStr(strToByteArray(str))).to.equal(str);
        expect(strToByteArray(str) instanceof Uint8Array);

    });

    it('Should convert a string to normal array, back again.', () => {

        const str = "holy mackerole";

        expect(arrayToStr(strToArray(str))).to.equal(str);
        expect(Array.isArray(strToArray(str)));

    });

});
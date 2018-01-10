import {strToByteArray, byteArrayToStr} from "./encoding";

describe('Text encoding', () => {

    it('Should convert a string to a byte array, back again.', () => {

        const str = "sample string";

        expect(byteArrayToStr(strToByteArray(str))).to.equal(str);
        expect(strToByteArray(str) instanceof Uint8Array);

    });

});
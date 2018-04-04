const {valToBase64, base64ToVal} = require('../base64convert');
const {isEqual} = require('lodash');
const assert = require('assert');

describe('base64 convert', () => {

	it('should convert numbers', () => {

		const val = 123;

		const str = valToBase64(val);

		assert(typeof str === typeof '');
		assert(base64ToVal(str) === val);


		const float = 3.1415926535;

		const str2 = valToBase64(float);

		assert(base64ToVal(str2) === float);


		const smallNum = 1.88e-13;

		const str3 = valToBase64(smallNum);

		assert(base64ToVal(str3) === smallNum);

	});


	it('should convert JSON objects', () => {

		const val = { a: { b: [1, 2, 3, 'hello'] }};

		const str = valToBase64(val);

		assert(typeof str === typeof '');
		assert(isEqual(base64ToVal(str), val));

	});


	it('should convert strings', () => {

		const val = 'hello\nworld!';

		const str = valToBase64(val);

		assert(typeof str === typeof '');
		assert(base64ToVal(str) === val);

	});


	it('should convert bytearray data', () => {

		const arr = new Uint8Array([1, 2, 3]);

		const str = valToBase64(arr);

		assert(typeof str === typeof '');

		const arr2 = base64ToVal(str);

		assert(arr2 instanceof ArrayBuffer);
		assert(isEqual(arr, new Uint8Array(arr2)));

	});

});
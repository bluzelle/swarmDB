const Buffer = global.Buffer || require('buffer').Buffer;
const {encode, decode} = require('base64-arraybuffer');

if(typeof btoa === 'undefined') {

  var btoa = str =>
    new Buffer(str, 'binary').toString('base64');

}

if (typeof atob === 'undefined') {

  var atob = base64 =>
    new Buffer(base64, 'base64').toString('binary');

}


const objPrefix = '0';

const objToBase64 = obj => 
	btoa(JSON.stringify(obj));

const base64ToObj = base64 => 
	JSON.parse(atob(base64));


// Bytearrays
const baPrefix = '1';

const baToBase64 = buffer => {

	if(!(buffer instanceof ArrayBuffer)) {

		// Unwrap from typed array

		buffer = buffer.buffer;

	}


	return encode(buffer);

};

const base64ToBa = decode;


const valToBase64 = val => {

	const type = typeof val;


	if(val instanceof ArrayBuffer || ArrayBuffer.isView(val)) {

		return baPrefix + baToBase64(val);

	}


	return objPrefix + objToBase64(val);

};

const base64ToVal = base64 => {

	const prefix = base64.substring(0, 1);
	base64 = base64.substring(1);


	if(prefix === baPrefix) {

		return base64ToBa(base64);

	}

	if(prefix === objPrefix) {

		return base64ToObj(base64);

	}

};



module.exports = {
    valToBase64,
    base64ToVal
};
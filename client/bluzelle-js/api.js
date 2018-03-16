const communication = require('./communication');
const {valToBase64, base64ToVal} = require('./base64convert');

const updateBase64 = communication.update;

const updateWithConversion = (key, value) =>
    updateBase64(key, valToBase64(value));


const readBase64 = communication.read;

const readWithConversion = key =>
    readBase64(key).then(
    	base64ToVal);


module.exports = Object.assign(communication, {
    update: updateWithConversion,
    read: readWithConversion
});
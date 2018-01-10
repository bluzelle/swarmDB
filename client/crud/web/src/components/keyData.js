export const isNew = keyData => !keyData.has('bytearray');
export const getPrefix = keyData => !isNew(keyData) && keyData.get('bytearray')[0];
export const addPrefix = (bytearray, prefix) => new Uint8Array([prefix, ...bytearray]);
export const getRaw = keyData => keyData.get('bytearray').subarray(1);
export const isNew = keyData => !keyData.has('bytearray');

export const getPrefix = keyData => !isNew(keyData) && keyData.get('bytearray')[0];

export const addPrefix = (arr, prefix) =>
    (arr instanceof Uint8Array) ? new Uint8Array([prefix, ...arr]) : [prefix, ...arr];

export const getRaw = keyData =>
    keyData.get('bytearray') instanceof Uint8Array
        ? keyData.get('bytearray').subarray(1)
        : keyData.get('bytearray').slice(1);
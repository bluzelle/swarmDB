const nodes = observable([]);

export const getNodes = () => nodes.toJS();

// Fake stuff down here
autorun(() => {
    Session.get('ready') &&
    [
        {address: '0x0000001', messages: 20889},
        {address: '0x0000002', messages: 20},
        {address: '0x0000003', messages: 20},
        {address: '0x0000004', messages: 20},
        {address: '0x0000005', messages: 20675},
        {address: '0x0000006', messages: 20},
        {address: '0x0000007', messages: 20},
        {address: '0x0000008', messages: 20},
        {address: '0x0000009', messages: 55678},
        {address: '0x0000010', messages: 20},
        {address: '0x0000011', messages: 20},
        {address: '0x0000012', messages: 20},
        {address: '0x0000013', messages: 18745},
        {address: '0x0000014', messages: 20},
        {address: '0x0000015', messages: 20},
        {address: '0x0000016', messages: 20},
        {address: '0x0000017', messages: 20},
        {address: '0x0000018', messages: 20}
    ].forEach((node, idx) => setTimeout(() => nodes.push(node), idx * 500));
});
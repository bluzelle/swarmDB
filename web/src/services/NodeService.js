import remove from 'lodash/remove'

const nodes = observable([]);

export const getNodes = () => nodes;

export const addNode = node => nodes.push(node);
export const removeNodeByAddress = address => remove(nodes, n => n.address === address);
export const clearNodes = () => remove(nodes);

autorun(() => {
    Session.get('ready') &&
    [
        {address: '0x0000001', messages: 20889, nodeState: 'new'},
        {address: '0x0000002', messages: 20, nodeState: 'new'},
        {address: '0x0000003', messages: 20, nodeState: 'alive'},
        {address: '0x0000004', messages: 20, nodeState: 'new'},
        {address: '0x0000005', messages: 20675, nodeState: 'dead'},
        {address: '0x0000006', messages: 20, nodeState: 'alive'},
        {address: '0x0000007', messages: 20, nodeState: 'alive'},
        {address: '0x0000008', messages: 20, nodeState: 'alive'},
        {address: '0x0000009', messages: 55678, nodeState: 'new'},
        {address: '0x0000010', messages: 20, nodeState: 'dead'},
        {address: '0x0000011', messages: 20, nodeState: 'alive'},
        {address: '0x0000012', messages: 20, nodeState: 'alive'},
        {address: '0x0000013', messages: 18745, nodeState: 'new'},
        {address: '0x0000014', messages: 20, nodeState: 'alive'},
        {address: '0x0000015', messages: 20, nodeState: 'alive'},
        {address: '0x0000016', messages: 20, nodeState: 'alive'},
        {address: '0x0000017', messages: 20, nodeState: 'dead'},
        {address: '0x0000018', messages: 20, nodeState: 'new'}
    ].forEach((node, idx) => setTimeout(() => addNode(node), idx * 100));
});


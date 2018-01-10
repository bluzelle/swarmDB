const NodeMessagesBtn = ({address}) => (
    <LinkBtn to={`/message-list/filtered-by-address/${address}`}>Messages</LinkBtn>
);

export default NodeMessagesBtn
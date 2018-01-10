const withParams = (Component) => props => <Component {...props} {...props.match.params} />;
export default withParams
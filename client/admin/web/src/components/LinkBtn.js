const LinkBtn = ({className = '', ...props}) => (
    <Link {...props} className={`btn btn-primary btn-xs ${className}`}>Messages</Link>
);

export default LinkBtn
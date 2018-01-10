export class ReflexFixed extends Component {
    render() {
        const {maxSize, minSize, children, ...props} = this.props;
        return <div {...props}>{children}</div>;
    }
}
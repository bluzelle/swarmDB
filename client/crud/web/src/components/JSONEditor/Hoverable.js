export class Hoverable extends Component {

    constructor(props) {
        super(props);

        this.state = {
            hovering: false
        };
    }

    render() {
        const {children} = this.props;

        return (
            <div style={{
                    padding: 5,
                    paddingRight: 0,
                    background: this.state.hovering ? '#F9F9F9' : '#FFFFFF'
                }}
                onMouseOver={() => this.setState({ hovering: true })}
                onMouseLeave={() => this.setState({ hovering: false })}>

                {
                    // Only works on single children
                    React.cloneElement(children, { hovering: this.state.hovering })
                }
            </div>
        );
    }
}
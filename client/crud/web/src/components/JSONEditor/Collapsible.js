export class Collapsible extends Component {
    constructor(props) {
        super(props);
        this.state = {collapsed: props.collapsed || false};
    }

    toggleCollapse() {
        this.setState({collapsed: !this.state.collapsed});
    }

    collapseTriangle() {
        return this.state.collapsed ? 'chevron-right' : 'chevron-down';
    }

    render() {
        const {children, label, buttons, preamble} = this.props;
        const {collapsed} = this.state;

        return (
            <React.Fragment>
                <div style={{ minHeight: 21 }}>
                    {preamble && <span style={{ marginRight: 5 }}>{preamble}:</span>}
                    <span onClick={this.toggleCollapse.bind(this)}>
                        <BS.Glyphicon glyph={this.collapseTriangle()}/> {label}
                    </span>
                    {buttons}
                </div>
                <div style={{
                    paddingLeft: 20,
                    background: 'white',
                    borderLeft: '1px solid #CCCCCC'
                }}>
                    {collapsed || children}
                </div>
            </React.Fragment>
        );
    }
}
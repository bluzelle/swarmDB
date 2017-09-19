import Nav from 'react-bootstrap/lib/Nav'
import NavItem from 'react-bootstrap/lib/NavItem'

class MainTabs extends Component {

    handleSelect(path) {
        this.props.history.push(path);
    }

    isActive(path) {
        return this.props.location.pathname === path
    }

    render() {
        return (
            <Nav bsStyle="tabs" onSelect={this.handleSelect.bind(this)}>
                <NavItem eventKey="/" active={this.isActive('/')}>Log</NavItem>
                <NavItem eventKey="/node-graph"  active={this.isActive('/node-graph')}>Node Graph</NavItem>
            </Nav>
        )
    }
}

export default withRouter(MainTabs)
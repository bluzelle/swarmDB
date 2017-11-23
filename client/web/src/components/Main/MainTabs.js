import Nav from 'react-bootstrap/lib/Nav'
import NavItem from 'react-bootstrap/lib/NavItem'

class MainTabs extends Component {

    handleSelect(path) {
        this.props.history.push(path);
    }

    isActive(path) {
        return path === '/' ? this.props.location === path : this.props.location.pathname.startsWith(path);
    }

    render() {
        return (
            <Nav bsStyle="tabs" onSelect={this.handleSelect.bind(this)}>
                <NavItem eventKey="/" active={this.isActive('/')}>Log</NavItem>
                <NavItem eventKey="/message-list" active={this.isActive('/message-list')}>Message List</NavItem>
                <NavItem eventKey="/node-list" active={this.isActive('/node-list')}>Node List</NavItem>
                <NavItem eventKey="/node-graph"  active={this.isActive('/node-graph')}>Node Graph</NavItem>
                <NavItem eventKey="/settings" active={this.isActive('/settings')}>Settings</NavItem>
                {
                    global.electron && <NavItem eventKey="/manage-daemons" active={this.isActive('/manage-daemons')}>Daemon Manager</NavItem>
                }
            </Nav>
        )
    }
}

export default withRouter(MainTabs)
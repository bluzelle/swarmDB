import CenterMiddle from './CenterMiddle'
import Panel from 'react-bootstrap/lib/Panel'
import Button from 'react-bootstrap/lib/Button'
import Header from '../Header'

@observer
export default class DaemonSelector extends Component {
    // go() {
    //     entryPointUrl.set(`${this.address.value}:${this.port.value}`);
    // }

    checkEnterKey(ev) {
        ev.keyCode === 13 && this.go();
    }

    componentDidMount() {
        this.address.focus();
    }

    componentWillMount() {
        this.setState({
            emulatorStarted: global.electron && electron.remote.require('./emulator/Emulator').wasStarted
        });
    }


    startEmulator() {
        electron.remote.require('./emulator/Emulator').start();
        this.setState({emulatorStarted: true});
    }



    render() {
        const {emulatorStarted} = this.state;
         return (
            <CenterMiddle>
                <Header/>
                <div onKeyUp={this.checkEnterKey.bind(this)}>
                    <Panel style={{marginTop: 20}} header={<h3>Choose a Bluzelle node</h3>}>
                        <div style={{width: 400}}>
                            <div style={{float: 'right', width: '15%'}}>
                                <label style={{display: 'block'}}>Port:</label>
                                <input type="text" tabIndex="2" ref={r => this.port = r} style={{width: '100%'}} defaultValue="8100" />
                            </div>
                            <div style={{width: '80%'}}>
                                <label style={{display: 'block'}}>Address:</label>
                                <input type="text" tabIndex="1" ref={r => this.address = r} style={{width: '80%'}} placeholder="address" defaultValue="127.0.0.1"/>
                            </div>
                            <div style={{marginTop: 10}}>
                                    <Button onClick={this.props.go} tabIndex="3">Go</Button>
                                {global.electron && <Button style={{marginLeft: 10}} disabled={emulatorStarted} onClick={this.startEmulator.bind(this)}>Start Emulator</Button>}
                            </div>
                        </div>
                    </Panel>
                </div>
            </CenterMiddle>
        );
    }
}



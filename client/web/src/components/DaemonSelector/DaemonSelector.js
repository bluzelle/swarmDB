import CenterMiddle from 'components/CenterMiddle'
import Panel from 'react-bootstrap/lib/Panel'
import Button from 'react-bootstrap/lib/Button'
import { socketState, entryPointUrl, closeCode, disconnect} from "../../services/CommunicationService";
import Header from 'components/Main/Header'

@observer
export default class DaemonSelector extends Component {
    go() {
        entryPointUrl.set(`${this.address.value}:${this.port.value}`);
    }

    checkEnterKey(ev) {
        ev.keyCode === 13 && this.go();
    }

    componentDidMount() {
        this.address.focus();
    }

    startEmulator() {
        electron.remote.require('./emulator/Emulator').start();
    }

    render() {
        return (
            <CenterMiddle>
                <div onKeyUp={this.checkEnterKey.bind(this)}>
                    <Header/>
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
                                    <Button onClick={this.go.bind(this)} tabIndex="3">Go</Button>
                                {global.electron && <Button style={{marginLeft: 10}} onClick={this.startEmulator.bind(this)}>Start Emulator</Button>}
                            </div>
                        </div>
                    </Panel>
                    <div style={{height: 20}}>
                        {/*closeCode.get() && closeCode.get() !== 1000 && <span style={{color: 'red'}}>Error: {closeCode.get()}</span>*/}
                    </div>
                </div>
            </CenterMiddle>
        );
    }
}



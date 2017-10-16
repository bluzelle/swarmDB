import logo from './logo-color.png'
import {daemonUrl, socketState, disconnect} from "../../services/CommunicationService";
import Button from 'react-bootstrap/lib/Button'

const Header = () => (
    <header>
        {socketState.get() === 'open' && (
            <div style={{float: 'right', padding: 10}}>
                <div>{daemonUrl.get()}</div>
                <div>
                <Button bsSize="xsmall" onClick={disconnect}>Disconnect</Button>
                </div>
            </div>
        )}
        <div style={{height: 50, padding: 5}}>
            <img src={logo}/>
        </div>
    </header>
);

export default observer(Header);




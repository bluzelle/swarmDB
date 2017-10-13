import {getLogEntries} from 'services/LogService'
import LogComponentView from './LogComponentView'

const LogComponent = observer(() => <LogComponentView logEntries={getLogEntries()} />);
export default LogComponent


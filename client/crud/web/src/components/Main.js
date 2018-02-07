import {Editor} from "./Editor";
import {getLocalDataStore} from '../services/DataService';
import {selectedKey, KeyList} from "./KeyList";
import {Header} from "./Header/Header";
import 'bootstrap/dist/css/bootstrap.css';
import {CommandControls} from "./CommandControls";

@observer
export class Main extends Component {
    render() {

        const obj = getLocalDataStore();

        return (
            <ReflexContainer style={{height: '100%'}}>
                <ReflexFixed>
                    <Header/>
                    <hr/>
                </ReflexFixed>
                <ReflexElement flex={1}>
                    <ReflexContainer orientation='vertical'>
                        <ReflexElement flex={0.4}>
                            <CommandControls/>

                            <hr/>

                            <KeyList obj={obj}/>
                        </ReflexElement>
                        <ReflexSplitter/>
                        <ReflexElement>
                            {obj.has(selectedKey.get()) && <Editor obj={obj}/>}
                        </ReflexElement>
                    </ReflexContainer>
                </ReflexElement>
            </ReflexContainer>
        );
    }
}
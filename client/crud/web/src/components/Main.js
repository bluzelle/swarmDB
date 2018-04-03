import {Editor} from "./Editor";
import {KeyList} from "./KeyList";
import {Header} from "./Header/Header";
import 'bootstrap/dist/css/bootstrap.css';
import {CommandControls} from "./CommandControls";


@observer
export class Main extends Component {
    render() {
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
                            <KeyList/>

                        </ReflexElement>
                        <ReflexSplitter/>
                        <ReflexElement>

                            <Editor/>

                        </ReflexElement>
                    </ReflexContainer>
                </ReflexElement>
            </ReflexContainer>
        );
    }
}
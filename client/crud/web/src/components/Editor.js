import {JSONEditor} from "./JSONEditor";
import {PlainTextEditor} from './PlainTextEditor';
import {FileEditor} from "./FileEditor/FileEditor";

import {activeValue} from '../services/CRUDService';


@observer
export class Editor extends Component {

    render() {

        if(activeValue.get() instanceof ArrayBuffer) {

            return <FileEditor/>;

        }


        const type = typeof activeValue.get();

        if(type === 'object') {

            return <JSONEditor/>;

        }


        if(type === 'string') {

            return <PlainTextEditor/>;

        }


        return <div></div>;

    }

};

import {RenderArray} from "../Arrays/RenderArray";
import {RenderObject} from "../Objects/RenderObject";
import {RenderField} from "./RenderField";
import {get} from '../../../util/mobXUtils';
import {isObservableArray} from 'mobx';
import {Hoverable} from "../Hoverable";

@observer
export class RenderTree extends Component {
    constructor(props) {
        super(props);

        this.state = {
            editing: false
        };
    }

    render() {
        const {obj, propName} = this.props;
        let r;

        // If array
        if (!this.state.editing && isObservableArray(get(obj, propName))) {
            r = (
                <RenderArray
                    {...this.props}
                    onEdit={() => this.setState({ editing: true })}/>
            );

        // If object
        } else if (!this.state.editing && typeof get(obj, propName) === 'object') {
            r = (
                <RenderObject
                    {...this.props}
                    onEdit={() => this.setState({ editing: true })}/>
            );

        // Standard datatypes
        } else {
            r = (
                <RenderField
                    {...this.props}
                    editing={this.state.editing}
                    onChange={() => this.setState({editing: false})}/>
            );
        }

        return (
            <span style={{ fontFamily: 'monospace ' }}>
                <Hoverable>
                    {r}
                </Hoverable>
            </span>
        );
    }
}
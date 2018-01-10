import {selectedKey} from "./KeyList";
import {executeContext} from "../../services/CommandQueueService";

@executeContext
export class RemoveButton extends Component {

    onRemove() {
        const {obj} = this.props;

        if (selectedKey.get() !== null) {
            const oldObj = obj.get(selectedKey.get());
            const oldKey = selectedKey.get();

            this.context.execute({
                doIt: () => {
                    obj.delete(oldKey);
                    selectedKey.set(null);
                },
                undoIt: () => {
                    obj.set(oldKey, oldObj);
                    selectedKey.set(oldKey);
                },
                onSave: () => ({[oldKey]: 'deleted'}),
                message: <span>Deleted <code key={1}>{oldKey}</code></span>
            });
        }
    }

    render() {
        return (
            <BS.Button
                style={{color: 'red'}}
                onClick={this.onRemove.bind(this)}>
                <BS.Glyphicon glyph='remove'/>
            </BS.Button>
        );
    }
}
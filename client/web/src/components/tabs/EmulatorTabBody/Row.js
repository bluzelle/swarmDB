import Button from 'react-bootstrap/lib/Button'
import invoke from 'lodash/invoke'

@observer
export default class Row extends Component {
    @observable editing;

    setValue() {
        const value = this.props.type === 'number' ? parseInt(this.input.value) : this.input.value;
        this.editing = false;
        invoke(this.props, 'setFn', value);
    }

    render() {
        const {label, type, id, value} = this.props;

        return (
            <Fixed style={{height: 43, lineHeight: '42px', marginBottom: 5, width: 500, borderBottomWidth: 1, borderBottomStyle: 'solid'}}>
                    <Layout type="">
                        <Fixed style={{width: 100}}>
                            <label>{label}</label>
                        </Fixed>
                        <Flex>
                            {this.editing ? (
                                <input style={{lineHeight: '20px'}} type={type} ref={r => this.input = r} defaultValue={value}/>
                            ) :
                                value
                            }
                        </Flex>
                        <Fixed style={{width: 200}}>
                            {this.editing ? (
                                [
                                    <Button className="pull-right" key="set-btn" bsSize="small" onClick={this.setValue.bind(this)}>Set</Button>,
                                    <Button className="pull-right" key="edit-btn" bsSize="small" onClick={() => this.editing = false} style={{marginRight: 10}}>Cancel</Button>
                                ]
                            ) : (
                                <Button className="pull-right" bsSize="small" onClick={() => this.editing = true}>Edit</Button>
                            )}
                        </Fixed>
                    </Layout>
            </Fixed>
        )
    }
}

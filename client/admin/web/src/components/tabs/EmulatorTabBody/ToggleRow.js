import Button from 'react-bootstrap/lib/Button'
import invoke from 'lodash/invoke'

@observer
export default class InputRow extends Component {

    componentWillMount() {
        this.setState({value: this.props.value});
    }

    toggle() {
        const newValue = !this.state.value;
        invoke(this.props, 'setFn', newValue);
        this.setState({value: newValue});
    }

    render() {
        const {label, type, id} = this.props;
        const {value} = this.state;
        return (
            <Fixed style={{height: 43, lineHeight: '42px', marginBottom: 5, width: 500, borderBottomWidth: 1, borderBottomStyle: 'solid'}}>
                <Layout type="">
                    <Fixed style={{width: 200}}>
                        <label>{label}</label>
                    </Fixed>
                    <Flex>
                        {value.toString()}
                    </Flex>
                    <Fixed style={{width: 200}}>
                        <Button className="pull-right" bsSize="small" onClick={this.toggle.bind(this)}>Toggle</Button>
                    </Fixed>
                </Layout>
            </Fixed>
        )
    }
}

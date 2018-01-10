import {Form, FormGroup} from 'react-bootstrap';
import {SelectedInput} from "./SelectedInput";

export class EditableField extends Component {
    constructor(props) {
        super(props);

        this.state = {
            formValue: props.val,
            formActive: false,
            hovering: false
        };
    }

    componentWillMount() {
        this.props.active && this.setState({ formActive: true });
    }

    componentWillReceiveProps(nextProps) {
        nextProps.active && this.setState({ formActive: true });
    }

    handleChange(event) {
        this.setState({
            formValue: event.target.value
        });
    }

    handleSubmit(event) {
        const {onChange} = this.props;
        event.preventDefault();

        this.setState({
            formActive: false
        });

        onChange(this.state.formValue);
    }

    validationState() {
        try {
            JSON.parse(this.state.formValue);
            return 'success';
        } catch(e) {
            return 'error';
        }
    }

    render() {
        const {val, renderVal, validateJSON} = this.props;
        const renderValWithDefault = renderVal || (i => i);

        return (
            <span onClick={e => {
                e.stopPropagation();
                this.setState({ formActive: true, hovering: false })
            }}>
              {this.state.formActive ?
                  <Form inline
                        style={{display: 'inline'}}
                        onSubmit={this.handleSubmit.bind(this)}>
                      <FormGroup
                          controlId='JSONForm'
                          validationState={validateJSON ? this.validationState() : null}>
                          <SelectedInput
                              type='text'
                              value={this.state.formValue}
                              onChange={this.handleChange.bind(this)}
                              onBlur={this.handleSubmit.bind(this)}
                          />
                      </FormGroup>
                  </Form>
                  : <span style={{
                      textDecoration: this.state.hovering ? 'underline' : 'none',
                      cursor: 'pointer'
                  }}
                          onMouseOver={() => this.setState({hovering: true})}
                          onMouseLeave={() => this.setState({hovering: false})}>
                      {renderValWithDefault(val)}
                  </span>}
              </span>
        );
    }
}
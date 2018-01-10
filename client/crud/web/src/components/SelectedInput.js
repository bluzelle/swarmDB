export class SelectedInput extends Component {
    componentDidMount() {
        this.input && this.input.select();
    }

    render() {
        return (
            <BS.FormControl
                type='text'
                {...this.props}
                inputRef={c => this.input = c}/>
        );
    }
}
export class FileSize extends Component {

    render() {
        
        const {numBytes} = this.props;

        if(numBytes < 1000) {
            return <span>{numBytes} bytes</span>;
        }

        if(numBytes < 1e6) {
            return <span>{Math.floor(numBytes / 1000)} KB</span>;
        }

        if(numBytes < 1e9) {
            return <span>{Math.floor(numBytes / 1e6)} MB</span>;
        }

        if(numBytes < 1e12) {
            return <span>{Math.floor(numBytes / 1e9)} GB</span>;y
        }
    }

}
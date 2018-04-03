import logo from './logo-color.png';

export const Header = () => (
    <div style={{
        textAlign: 'center',
        marginTop: 10,
        marginBottom: 25
    }}>
        <img src={logo}/>
        <h1 style={{
            display: 'inline-block',
            fontWeight: 'bold',
            fontFamily: 'Courier New',
            color: '#32658A',
            marginLeft: 35,
            marginBottom: -3,
            verticalAlign: 'bottom',
            fontSize: 40
        }}>Database Editor</h1>
    </div>
);
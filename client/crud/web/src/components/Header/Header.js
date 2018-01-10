import logo from './logo-color.png';

export const Header = () => (
    <div style={{textAlign: 'center'}}>
        <img src={logo}/>
        <h1 style={{
            display: 'inline-block',
            fontWeight: 'bold',
            marginLeft: 35,
            marginBottom: '-3px',
            verticalAlign: 'bottom',
            fontSize: 40
        }}>Database Editor</h1>
    </div>
);
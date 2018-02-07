import logo from './logo-color.png'

const Header = () => (
    <header>
        <div style={{height: 50, padding: 5}}>
            <img src={logo}/>
        </div>
    </header>
);

export default observer(Header);




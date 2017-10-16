const path = require('path');
const webpack = require('webpack');
const BeepPlugin = require('webpack-beep-plugin');

const PROD = process.env.NODE_ENV === 'production';
PROD && console.log('----------- Compiling for production ----------');


module.exports = () => ({
    entry: {
        index: path.resolve('./index.js')
    },
    output: {
        path: path.resolve('../dist/generated/js'),
        filename: '[name].js'
    },
    devtool: PROD ? '' : 'inline-source-map',
    module: {
        rules: [
            {
                test: /\.js$/,
                exclude: /(node_modules)/,
                use: {
                    loader: 'babel-loader',
                    options: {
                        presets: ['env', 'react', 'stage-3', 'stage-2'],
                        plugins: ['transform-decorators-legacy']
                    }
                }
            },
            {
                test: /\.css$/,
                use: ['style-loader', 'css-loader?sourceMap']
            },
            {
                test: /\.(jpe?g|png|gif|svg)$/i,
                use: 'url-loader'
            },
            {
                test: /\.woff(2)?(\?v=[0-9]\.[0-9]\.[0-9])?$/,
                use: {
                    loader: "url-loader",
                    options: {
                        mimetype: 'application/font-woff'
                    }
                }
            },
            {
                test: /\.(ttf|eot)(\?v=[0-9]\.[0-9]\.[0-9])?$/,
                use: {
                    loader: "url-loader"
                }
            }
        ]
    },
    resolve: {
        alias: {
            components: path.resolve('components'),
            services: path.resolve('services'),
            stores: path.resolve('stores'),
            src: path.resolve(''),
            constants: path.resolve('constants')
        }
    },
    plugins: [
        new webpack.ProvidePlugin({
            React: 'react',

            Component: 'components/Component',

            LinkBtn: ['components/LinkBtn', 'default'],
            withParams: ['components/withParams', 'default'],

            Layout: 'react-layout-pane/lib/Layout',
            Fixed: 'react-layout-pane/lib/Fixed',
            Flex: 'react-layout-pane/lib/Flex',

            withRouter: ['react-router-dom/withRouter', 'default'],
            Link: ['react-router-dom/Link', 'default'],
            Route: ['react-router-dom/Route', 'default'],
            Switch: ['react-router-dom/Switch', 'default'],

            observable: ['mobx', 'observable'],
            extendObservable: ['mobx', 'extendObservable'],
            observer: ['mobx-react', 'observer'],
            autorun: ['mobx', 'autorun'],
            untracked: ['mobx', 'untracked'],
            computed: ['mobx', 'computed'],
            transaction: ['mobx', 'transaction'],
            action: ['mobx', 'action'],

            Session: [path.resolve('stores/SessionStore'), 'default'],

            sinon: 'sinon',

            '$j': 'jquery',

            Maybe: ['monet', 'Maybe'],
            Either: ['monet', 'Either']


        }),
        new BeepPlugin(),
        new webpack.NamedModulesPlugin()
    ]
});



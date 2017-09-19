const path = require('path');
const webpack = require('webpack');
const BeepPlugin = require('webpack-beep-plugin');
const glob = require('glob');

const testFiles = glob.sync("**/*.spec.js").filter(filename => /node_modules/.test(filename) === false);

module.exports = {
    entry: {
        index: './index.js',
        test: './test.js'
    },
    output: {
        path: path.resolve('../dist/generated/js'),
        filename: '[name].js'
    },
    devtool: 'inline-source-map',
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
            src: path.resolve('')
        }
    },
    plugins: [
        new webpack.ProvidePlugin({
            React: 'react',

            Component: 'components/Component',

            Layout: 'react-layout-pane/lib/Layout',
            Fixed: 'react-layout-pane/lib/Fixed',
            Flex: 'react-layout-pane/lib/Flex',

            withRouter: ['react-router-dom/withRouter', 'default'],
            Link: ['react-router-dom/link', 'default'],
            Route: ['react-router-dom/Route', 'default'],
            Switch: ['react-router-dom/Switch', 'default'],

            observable: ['mobx', 'observable'],
            extendObservable: ['mobx', 'extendObservable'],
            observer: ['mobx-react', 'observer'],
            autorun: ['mobx', 'autorun'],
            computed: ['mobx', 'computed'],

            Session: [path.resolve('stores/SessionStore'), 'default']


        }),
        new BeepPlugin(),
        new webpack.NamedModulesPlugin()
    ]
};

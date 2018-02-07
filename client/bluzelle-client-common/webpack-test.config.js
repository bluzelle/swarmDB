const webpack = require('webpack');

const config = require('bluzelle-client-common/webpack-base-config.js')();
const path = require('path');

config.entry = {test: path.resolve('./test.js')};

config.resolve.alias = Object.assign({
    'bluzelle-client-common/services/CommunicationService': path.resolve('./mocks/MockCommunicationService')
}, config.resolve.alias);

config.plugins.push(new webpack.ProvidePlugin({
    waitFor: 'promise-waitfor'
}));

module.exports = config;


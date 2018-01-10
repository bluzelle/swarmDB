const config = require('bluzelle-client-common/webpack-test-config.js')();

config.module.rules.push({
    test: /\.js$|\.jsx$/,
    use: {
        loader: 'istanbul-instrumenter-loader',
        options: {esModules: true}
    },
    enforce: 'post',
    exclude: [/node_modules|\.specs\.js$/, /test\.js/],
});

module.exports = config;
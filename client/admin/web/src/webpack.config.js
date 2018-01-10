const config = require('bluzelle-client-common/webpack-base-config.js')();
const os = require('os');
const WebpackShellPlugin = require('webpack-shell-plugin');


const IS_WINDOWS = os.platform() === 'win32';

// IS_WINDOWS || config.plugins.push(new WebpackShellPlugin({
//         onBuildEnd:['./copy-to-daemon.sh']
//     })
// );


module.exports = config;

module.exports = () => ({
    entry: './src/index.js',
    output: {
        path: __dirname + '/dist',
        filename: 'bundle.js'
    },
    module: {
        rules: [
            {
                test: /\.js$/,
                include: (path) => !(/node_modules/.test(path)),

                use: {
                    loader: 'babel-loader',
                    options: {
                        presets: ['env', 'react', 'stage-3', 'stage-2']
                    }
                }
            }
        ]
    }
});
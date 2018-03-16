mocha.setup('bdd');
mocha.setup({timeout: 5000});

const requireContext = require.context('../../test', true, /\.test\.js/);
requireContext.keys().forEach(requireContext);


mocha.run();
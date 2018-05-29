#!/usr/bin/env bash

cd ../..

git clone git@github.com:bluzelle/bluzelle-js.git

cd bluzelle-js

yarn

if [ "$1" == "setup" ]; then
printf "*************************************
Copying template configuration files.

Please add an Etherscan api key at minimum to the config files at swarmDB/build/*.json, and then run this script again with no arguments.
*************************************\n"
    yarn setup-daemon
else
    echo "**** Running tests with your configuration files. ****"
    yarn test-integration
fi


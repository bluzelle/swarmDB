#!/usr/bin/env bash
PLATFORM=$1
PACKAGE_DIR=$2

cd $WORKSPACE/client/web
yarn
yarn dev-compile

cd $WORKSPACE/client/desktop
yarn
yarn package-$PLATFORM
tar -czvC $WORKSPACE/client/desktop/dist -f $WORKSPACE/bluzelle.tar.gz $PACKAGE_DIR
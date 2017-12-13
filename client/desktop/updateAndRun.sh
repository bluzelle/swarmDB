#!/usr/bin/env bash
BRANCH=$1
git fetch origin
git checkout origin/$BRANCH

cd emulator
yarn
cd ..
cd web
yarn
yarn dev-compile
cd ..
cd desktop
yarn
yarn start

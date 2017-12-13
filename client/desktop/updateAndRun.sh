#!/usr/bin/env bash
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

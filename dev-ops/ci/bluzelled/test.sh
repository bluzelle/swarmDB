#!/usr/bin/env bash

cd $WORKSPACE
rm -rf build
mkdir build
cd build
cmake .. &&
make bluzelled_test &&
cd bluzelled &&
./bluzelled_test



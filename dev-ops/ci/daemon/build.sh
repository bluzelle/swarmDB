#!/usr/bin/env bash

setup_makefiles_in_build_folder ( )
{
    workspace=$1
    cd $workspace
    rm -rf build
    mkdir build

    cd $workspace/build
    cmake ..
}

make_valid_target ( )
{
    target=$1
    make ${target}
}

###############################################################################
if [ -z "$1" ]
then
    target=$MAKE_TARGET
else
    target=$1
fi

valid_targets=("the_db" "the_db_test")
if [[ ${valid_targets[@]} =~ $target ]]
then
    if [ -d "$WORKSPACE" ]
    then
        setup_makefiles_in_build_folder $WORKSPACE
        make_valid_target $target
    else
        echo "Please define the WORKSPACE environment variable."
    fi
else
    echo "Please specify a valid target to make: the_db_test or the_db"
fi

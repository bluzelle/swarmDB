#!/usr/bin/env bash
if [ -f $WORKSPACE/pid ]; then
    kill -9 `cat $WORKSPACE/pid`
fi
#PID=`ps ax |grep 'bluzelled' | grep -v grep | cut -d' ' -f2`
#kill -9 $PID || true

cd $WORKSPACE
rm -rf build
mkdir build
cd $WORKSPACE/build
cmake .. &&
make bluzelled &&

BUILD_ID=dontKillMe
chmod 755 $WORKSPACE/build/bluzelled/bluzelled
/usr/local/sbin/daemonize -c $WORKSPACE/build/bluzelled -e $WORKSPACE/daemon-error -o $WORKSPACE/daemon-stdout -p $WORKSPACE/pid -E BUILD_ID=dontKillMe $WORKSPACE/build/bluzelled/bluzelled




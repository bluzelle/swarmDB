#!/usr/bin/env bash
PID=`ps ax |grep '/var/lib/jenkins/workspace/Kepler/Emulator/client/web/node_modules/.bin/ws' | grep -v grep | cut -d' ' -f2`

kill -9 $PID || true

cd $WORKSPACE/client/web
yarn
cd $WORKSPACE/client/web/src
webpack
cd $WORKSPACE/client/web

/usr/local/sbin/daemonize -c $WORKSPACE/client/web -e $WORKSPACE/daemon-error -o $WORKSPACE/daemon-stdout -p $WORKSPACE/pid -E BUILD_ID=dontKillMe `which yarn` start --port 3004


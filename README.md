# SwarmDB

[![Build Status](https://travis-ci.org/bluzelle/swarmDB.svg?branch=devel)](https://travis-ci.org/bluzelle/swarmDB)
[![License](https://img.shields.io/:license-AGPLv3-blue.svg?style=flat-square)](https://github.com/bluzelle/swarmDB/blob/devel/LICENSE)
[![Twitter](https://img.shields.io/badge/twitter-@bluzelle-blue.svg?style=flat-square)](https://twitter.com/BluzelleHQ)
[![Slack chat](https://img.shields.io/badge/slackchat-join%20chat-brightgreen.svg?style=flat-square)](https://bluzelle-swarm.slack.com)

## ABOUT SWARMDB

Bluzelle brings together the sharing economy and token economy. Bluzelle enables people to rent out their computer storage space to earn a token while dApp developers pay with a token to have their data stored and managed in the most efficient way.

## Getting Started

If you want to deploy your swarm immediately you can use our docker-compose quickstart instructions:

Docker Swarm Deploy

1. Download sample [peerlist.json](https://gist.github.com/amastracci/b6144d32bd790cf1a93e3c498a9d46cc)
2. Add your swarm machines' IP to the "host" field of each peer in peerlist.json
3. Host your peerlist.json at a publically accessable URL (must be non-https and non-redirecting)
4. Download sample [docker-compose.yml](https://gist.githubusercontent.com/amastracci/5e34d586c6a682e0eb91dc2ead9b5ed8/raw/ea259cadf1021b06e7b84ffe9f73143e119bee8d/docker-compose.yml)
5. Add your public URL to "SWARM_BOOTSTRAP_URL" to each service in docker-compose.yml
6. Run 'docker compose up' in the same directory of your docker-compose.yml

## Building from source 

### BOOST

Open up a console and install the capatible version of Boost:

```
ENV BOOST_VERSION="1.67.0"
ENV BOOST_INSTALL_DIR="~/myboost"

mkdir -p ~/myboost
toolchain/install_boost.sh
```
This will result in a custom Boost install at ```~/myboost/1_67_0/```that will not collide with your system Boost.

### Other dependencies

#### OSX

Brew has everything that you will need to build

```
brew update && brew install jsoncpp && brew upgrade cmake
```

#### Ubuntu

##### CMAKE

```
mkdir -p ~/mycmake
curl -L http://cmake.org/files/v3.11/cmake-3.11.0-Darwin-x86_64.tar.gz | tar -xz -C ~/mycmake --strip-components=1
```

Again, this will result in a custom cmake install into ```~/mycmake/``` and will not overload the system cmake.

```
sudo apt-get install pkg-config libjsoncpp-dev 
```

### ccache

If available, cmake will attempt to use ccache (https://ccache.samba.org) to *drastically* speed up compilation. 

#### OSX 

```brew install ccache```

### Ubuntu

```sudo apt-get install ccache```

## BUILDING THE DAEMON

### CLion

Ensure that you set your cmake args to pass in:

```
-DBOOST_ROOT:PATHNAME=~/myboost/1_67_0/
```
The project root directly can be directly imported into CLion.

### CLI

Here are the steps to build the daemon and unit test application from the command line:
```
mkdir build
cd build
~/mycmake/cmake -DBOOST_ROOT:PATHNAME=~/myboost/1_67_0/ ..
make install
````

## RUNNING THE APPLICATION

Steps to setup testing environment:

Build your Daemon in a directory named daemon-build in the same level as the swarmclient-js repo directory. 
Create each of the json files below in daemon-build/output/, where the swarm executable resides. (bluzelle1.json, bluzelle2.json, bluzelle3.json, peers.json)

Config files for Daemon:
```
// bluzelle1.json
{
  "listener_address" : "127.0.0.1",
  "listener_port" : 50000,
  "ethereum" : "0x006eae72077449caca91078ef78552c0cd9bce8f",
  "bootstrap_file" : "./peers.json",
  "uuid" : "60ba0788-9992-4cdb-b1f7-9f68eef52ab9",
  "debug_logging" : true,
  "log_to_stdout" : false
 
}
 
// bluzelle2.json
{
  "listener_address" : "127.0.0.1",
  "listener_port" : 50001,
  "ethereum" : "0x006eae72077449caca91078ef78552c0cd9bce8f",
  "bootstrap_file" : "./peers.json",
  "uuid" : "c7044c76-135b-452d-858a-f789d82c7eb7",
  "debug_logging" : true,
  "log_to_stdout" : true
 
}
 
// bluzelle3.json
{
  "listener_address" : "127.0.0.1",
  "listener_port" : 50002,
  "ethereum" : "0x006eae72077449caca91078ef78552c0cd9bce8f",
  "bootstrap_file" : "./peers.json",
  "uuid" : "3726ec5f-72b4-4ce6-9e60-f5c47f619a41",
  "debug_logging" : true,
  "log_to_stdout" : false
 
}
 
// A unique ID (uuid) must be specified for each daemon.
// debug_logging is an optional setting (default is false)
// listener address and port must match peer list
 
 
// peers.json
[
  {"name": "peer1", "host": "127.0.0.1", "port": 50000, "uuid" : "60ba0788-9992-4cdb-b1f7-9f68eef52ab9"},
  {"name": "peer2", "host": "127.0.0.1",  "port": 50001, "uuid" : "c7044c76-135b-452d-858a-f789d82c7eb7"},
  {"name": "peer3", "host": "127.0.0.1",  "port": 50002, "uuid" : "3726ec5f-72b4-4ce6-9e60-f5c47f619a41"}
]
 
```

Start your cluster from the daemon-build/output directory run:

```
swarm -c daemon-build/output -c bluzelle1.json
swarm -c daemon-build/output -c bluzelle2.json
swarm -c daemon-build/output -c bluzelle3.json
```

## Testing Locally

### NPM and WSCAT installation

#### Install npm if you don't have it with
```
sudo apt-get install npm
```
#### Update npm to the latest
```
sudo npm install npm@latest -g
```
#### Install wscat 
```
sudo npm install wscat -g
```
#### WSCAT installation without NPM
```
sudo apt-get install node-ws
```

### Sample WebSocker Calls


```
wscat -c 13.78.131.94:51011
connected (press CTRL+C to quit)

> {"bzn-api" : "crud","cmd" : "create","data" :{"key" : "key1","value" : "I2luY2x1ZGUgPG1vY2tzL21vY2tfbm9kZV9iYXNlLmhwcD4NCiNpbmNsdWRlIDxtb2Nrcy9tb2NrX3Nlc3Npb25fYmFzZS5ocHA+DQojaW5jbHVkZSA8bW9ja3MvbW9ja19yYWZ0X2Jhc2UuaHBwPg0KI2luY2x1ZGUgPG1vY2tzL21vY2tfc3RvcmFnZV9iYXNlLmhwcD4NCg=="},"db-uuid" : "80174b53-2dda-49f1-9d6a-6a780d4cceca","request-id" : 85746}
< {
   "data" : {
      "leader-host" : "13.78.131.94",
      "leader-id" : "92b8bac6-3242-452a-9090-1aa48afd71a3",
      "leader-name" : "swarm01",
      "leader-port" : 51010
   },
   "error" : "NOT_THE_LEADER",
   "request-id" : 85746
}

disconnected

wscat -c 13.78.131.94:51010
connected (press CTRL+C to quit)
> {"bzn-api" : "crud","cmd" : "create","data" :{"key" : "key1","value" : "I2luY2x1ZGUgPG1vY2tzL21vY2tfbm9kZV9iYXNlLmhwcD4NCiNpbmNsdWRlIDxtb2Nrcy9tb2NrX3Nlc3Npb25fYmFzZS5ocHA+DQojaW5jbHVkZSA8bW9ja3MvbW9ja19yYWZ0X2Jhc2UuaHBwPg0KI2luY2x1ZGUgPG1vY2tzL21vY2tfc3RvcmFnZV9iYXNlLmhwcD4NCg=="},"db-uuid" : "80174b53-2dda-49f1-9d6a-6a780d4cceca","request-id" : 85746}
< {
   "request-id" : 85746
}

disconnected

wscat -c 13.78.131.94:51010
connected (press CTRL+C to quit)
> {"bzn-api" : "crud","cmd" : "has","data" :{"key" : "key0"},"db-uuid" : "80174b53-2dda-49f1-9d6a-6a780d4cceca","request-id" : 85746}
< {
   "data" : {
      "key-exists" : true
   },
   "request-id" : 85746
}
```
## FULL API Documentation

https://bluzelle.github.io/api/



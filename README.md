# SwarmDB

[![Build Status](https://travis-ci.org/bluzelle/swarmDB.svg?branch=devel)](https://travis-ci.org/bluzelle/swarmDB)
[![License](https://img.shields.io/:license-AGPLv3-blue.svg?style=flat-square)](https://github.com/bluzelle/swarmdb/blob/master/LICENSE)
[![Twitter](https://img.shields.io/badge/twitter-@bluzelle-blue.svg?style=flat-square)](https://twitter.com/bluzelle)
[![Slack chat](https://img.shields.io/badge/slackchat-join%20chat-brightgreen.svg?style=flat-square)](https://bluzelle-swarm.slack.com)

## ABOUT SWARMDB

Bluzelle brings together the sharing economy and token economy. Bluzelle enables people to rent out their computer storage space to earn a token while dApp developers pay with a token to have their data stored and managed in the most efficient way.

## Getting Started

If you want to deploy your swarm immediately you can use our docker-compose quickstart instructions:

Docker Swarm Deploy

1. Download sample [peerlist.json](https://gist.github.com/amastracci/b6144d32bd790cf1a93e3c498a9d46cc)
2. Add your swarm machines' IP to the "host" field of each peer in peerlist.json
3. Host your peerlist.json at a publically accessable URL
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

Again this will result in a customer cmake install into ```~/mycmake/```

```
sudo apt-get install pkg-config libjsoncpp-dev 
```

### ccache

If available, cmake will attempt to use ccache (https://ccache.samba.org) to *drastically* speed up compilation. 

#### OSX 

```brew install ccache```

### Ubuntu

```sudo apt-get install ccache```


## NPM and WSCAT installation

Install npm if you don't have it with
```
sudo apt-get install npm
```
Update npm to the latest
```
sudo npm install npm@latest -g
```
Install wscat 
```
sudo npm install wscat -g
```
WSCAT installation without NPM
```
sudo apt-get install node-ws
```

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
make
````

## RUNNING THE APPLICATION


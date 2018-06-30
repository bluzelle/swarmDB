# SwarmDB

[![Build Status](https://travis-ci.org/bluzelle/swarmDB.svg?branch=devel)](https://travis-ci.org/bluzelle/swarmDB)
[![License](https://img.shields.io/:license-AGPLv3-blue.svg?style=flat-square)](https://github.com/bluzelle/swarmDB/blob/devel/LICENSE)
[![Twitter](https://img.shields.io/badge/twitter-@bluzelle-blue.svg?style=flat-square)](https://twitter.com/BluzelleHQ)
[![Gitter chat](https://img.shields.io/gitter/room/nwjs/nw.js.svg?style=flat-square)](https://gitter.im/bluzelle)

## ABOUT SWARMDB

Bluzelle brings together the sharing economy and token economy. Bluzelle enables people to rent out their computer storage space to earn a token while dApp developers pay with a token to have their data stored and managed in the most efficient way.
  
## Getting started with Docker

If you want to deploy your swarm immediately you can use our docker-compose quickstart instructions:

### Install Docker

[Docker Installation Guide](https://docs.docker.com/install/)

1. Setup a local docker-compose swarm with the instructions found [here](https://github.com/bluzelle/docker-swarm-deploy)
2. Run `docker-compose up` in the same directory of your docker-compose.yml. This command will initialize the swarm within your local docker-machine. Full docker-compose documentation can be found [here](https://docs.docker.com/compose/)
3. Nodes are available on localhost port 51010-51012
4. [Connect a test websocket client](https://github.com/bluzelle/swarmDB#testing-locally) with our [websocket API](https://bluzelle.github.io/api/#websocket-api)
5. Create a node server application using our node.js [library](https://github.com/bluzelle/bluzelle-js) and [API](https://bluzelle.github.io/api/)
6. `CTRL-C` to terminate the docker-compose swarm

## Getting started building from source

### Installation - macOSX

##### Boost

```
$ export BOOST_VERSION="1.67.0"
$ export BOOST_INSTALL_DIR="~/myboost"

$ mkdir -p ~/myboost
$ toolchain/install-boost.sh
```

This will result in a custom Boost install at ```~/myboost/1_67_0/```that will not collide with your system's Boost.


##### Other dependencies (Protobuf, CMake)

```
$ brew update && brew install protobuf && brew upgrade cmake
```

##### ccache (Optional) 

If available, cmake will attempt to use ccache (https://ccache.samba.org) to *drastically* speed up compilation. 

```
$ brew install ccache
```

### Installation - Ubuntu

##### Boost

Open up a console and install the compatible version of Boost:

```
$ ENV BOOST_VERSION="1.67.0"
$ ENV BOOST_INSTALL_DIR="~/myboost"

$ mkdir -p ~/myboost
$ toolchain/install-boost.sh
```
This will result in a custom Boost install at ```~/myboost/1_67_0/```that will not overwrite your system's Boost.


##### CMake

```
$ mkdir -p ~/mycmake
$ curl -L http://cmake.org/files/v3.11/cmake-3.11.0-Darwin-x86_64.tar.gz | tar -xz -C ~/mycmake --strip-components=1
```
Again, this will result in a custom cmake install into ```~/mycmake/``` and will not overwrite your system's cmake.

##### Protobuf (Ver. 3 or greater)

```
$ sudo apt-get install pkg-config protobuf-compiler libprotobuf-dev
```

##### ccache (Optional)

If available, cmake will attempt to use ccache (https://ccache.samba.org) to *drastically* speed up compilation. 

```
$ sudo apt-get install ccache
```

### Building the Daemon with CLion IDE

Ensure that you set your cmake args to pass in:

```
-DBOOST_ROOT:PATHNAME=~/myboost/1_67_0/
```
The project root can be directly imported into CLion.

### Building the Daemon from Command Line Interface (CLI)

Here are the steps to build the Daemon and unit test application from the command line:

#### OSX

```
$ mkdir build
$ cd build
$ cmake -DBOOST_ROOT:PATHNAME=~/myboost/1_67_0/ ..
$ sudo make install
```

#### Ubuntu

```
$ mkdir build
$ cd build
$ ~/mycmake/cmake -DBOOST_ROOT:PATHNAME=~/myboost/1_67_0/ ..
$ sudo make install
```

### Deploying the Daemons 

#### Steps to setup Daemon configuration files:

1. Create each of the JSON files below in swarmDB/build/output/, where the swarm executable resides. (bluzelle.json, bluzelle2.json, bluzelle3.json, peers.json).
2. Create an account with Etherscan: https://etherscan.io/register
3. Create an Etherscan API KEY by clicking Developers -> API-KEYs.
4. Add your Etherscan API KEY Token to the configuration files.
5. Modify the `listener_address` to use your local interface IP.
```
$ ifconfig en1
en1: flags=8863<UP,BROADCAST,SMART,RUNNING,SIMPLEX,MULTICAST> mtu 1500
	inet6 fe80::1837:c97f:df86:c36f%en1 prefixlen 64 secured scopeid 0xa
-->>inet 192.168.0.34 netmask 0xffffff00 broadcast 192.168.0.255
	nd6 options=201<PERFORMNUD,DAD>
	media: autoselect
	status: active
```

In the above case, the IP address of the local interface is `192.168.0.34`.

If you do not see `inet <ipaddress>`, run `ifconfig` and comb through manually to find your local IP address.

6. Modify the `ethereum` address to be an Ethereum mainnet address that contains tokens or use the sample address provided below.

**Requirements:**
- You must provide a valid Ethereum address with a balance > 0 and an Etherscan API key.
- A unique ID (uuid) must be specified for each daemon.
- Listener address and port must match peer list.

Configuration files for Daemon:
```
// debug_logging is an optional setting (default is false)

// bluzelle.json
{
  "listener_address" : "127.0.0.1",
  "listener_port" : 50000,
  "ethereum" : "0xddbd2b932c763ba5b1b7ae3b362eac3e8d40121a",
  "ethereum_io_api_token" : "**********************************",
  "bootstrap_file" : "./peers.json",
  "uuid" : "60ba0788-9992-4cdb-b1f7-9f68eef52ab9",
  "debug_logging" : true,
  "log_to_stdout" : false
}
 
// bluzelle2.json
{
  "listener_address" : "127.0.0.1",
  "listener_port" : 50001,
  "ethereum" : "0xddbd2b932c763ba5b1b7ae3b362eac3e8d40121a",
  "ethereum_io_api_token" : "**********************************",
  "bootstrap_file" : "./peers.json",
  "uuid" : "c7044c76-135b-452d-858a-f789d82c7eb7",
  "debug_logging" : true,
  "log_to_stdout" : true
}
 
// bluzelle3.json
{
  "listener_address" : "127.0.0.1",
  "listener_port" : 50002,
  "ethereum" : "0xddbd2b932c763ba5b1b7ae3b362eac3e8d40121a",
  "ethereum_io_api_token" : "**********************************",
  "bootstrap_file" : "./peers.json",
  "uuid" : "3726ec5f-72b4-4ce6-9e60-f5c47f619a41",
  "debug_logging" : true,
  "log_to_stdout" : true
}
 
// peers.json
[
  {"name": "peer1", "host": "127.0.0.1", "port": 50000, "http_port: : 8080, "uuid" : "60ba0788-9992-4cdb-b1f7-9f68eef52ab9"},
  {"name": "peer2", "host": "127.0.0.1", "port": 50001, "http_port: : 8081, "uuid" : "c7044c76-135b-452d-858a-f789d82c7eb7"},
  {"name": "peer3", "host": "127.0.0.1", "port": 50002, "http_port: : 8082, "uuid" : "3726ec5f-72b4-4ce6-9e60-f5c47f619a41"}
]
 
```

7. Deploy your swarm of Daemons. From the swarmDB/build/output/ directory, run:

```
$ ./swarm -c bluzelle.json
$ ./swarm -c bluzelle2.json
$ ./swarm -c bluzelle3.json
```

## Integration Tests With Bluzelle's Javascript Client

### Installation - macOSX


#### Homebrew

```
$ /usr/bin/ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"
```

#### Node

```
$ brew install node
```
#### Yarn
```
$ brew install yarn
```
### Installation - Ubuntu

#### NPM
```
$ sudo apt-get install npm
```
#### Update NPM
```
$ sudo npm install npm@latest -g
```
#### Yarn
```
$ curl -sS https://dl.yarnpkg.com/debian/pubkey.gpg | sudo apt-key add -
echo "deb https://dl.yarnpkg.com/debian/ stable main" | sudo tee /etc/apt/sources.list.d/yarn.list

$ sudo apt-get update && sudo apt-get install yarn
```
### Running Integration Tests

Script to clone *bluzelle-js* repository and copy template configuration files or run tests with your configuration files.  
```
$ qa/integration-tests setup // Sets up template configuration files
```
```
$ qa/integration-tests // Runs tests with configuration files you've created
```
## Testing Locally
```
$ cd scripts

Follow instructions in readme.md 
```
#### Connectivity Test
```
$ ./crud -n localhost:50000 ping
Sending : 
{
	"bzn-api": "ping"
}
------------------------------------------------------------

Response: 
{
	"bzn-api" : "pong"
}

------------------------------------------------------------
```
#### Create
```
$ ./crud -n localhost:50000 create -u myuuid -k mykey -v myvalue
  Sending: db {
    header {
      db_uuid: "myuuid"
      transaction_id: 1149205427773053859
    }
    create {
      key: "mykey"
      value: "myvalue"
    }
  }
  
  ------------------------------------------------------------
  
  redirecting to leader at 127.0.0.1:50002...
  
  Sending: db {
    header {
      db_uuid: "myuuid"
      transaction_id: 8606810256052859786
    }
    create {
      key: "mykey"
      value: "myvalue"
    }
  }
  
  ------------------------------------------------------------
  
  Response: 
  header {
    db_uuid: "myuuid"
    transaction_id: 8606810256052859786
  }
  
  ------------------------------------------------------------
```
#### Read
```
$ ./crud -n localhost:50000 read -u myuuid -k mykey
Sending: db {
  header {
    db_uuid: "myuuid"
    transaction_id: 638497919636113033
  }
  read {
    key: "mykey"
  }
}

------------------------------------------------------------

Response: 
header {
  db_uuid: "myuuid"
  transaction_id: 638497919636113033
}
resp {
  value: "myvalue"
}

------------------------------------------------------------
```
#### Update
```
$ ./crud -n localhost:50000 update -u myuuid -k mykey -v mynewvalue
Sending: db {
  header {
    db_uuid: "myuuid"
    transaction_id: 7847882878681328930
  }
  update {
    key: "mykey"
    value: "mynewvalue"
  }
}

------------------------------------------------------------

redirecting to leader at 127.0.0.1:50002...

Sending: db {
  header {
    db_uuid: "myuuid"
    transaction_id: 2491234936151888566
  }
  update {
    key: "mykey"
    value: "mynewvalue"
  }
}

------------------------------------------------------------

Response: 
header {
  db_uuid: "myuuid"
  transaction_id: 2491234936151888566
}

------------------------------------------------------------
```
#### Delete
```
$ ./crud -n localhost:50000 delete -u myuuid -k mykey
Sending: db {
  header {
    db_uuid: "myuuid"
    transaction_id: 8470321215009858819
  }
  delete {
    key: "mykey"
  }
}

------------------------------------------------------------

redirecting to leader at 127.0.0.1:50002...

Sending: db {
  header {
    db_uuid: "myuuid"
    transaction_id: 8085312586421869529
  }
  delete {
    key: "mykey"
  }
}

------------------------------------------------------------

Response: 
header {
  db_uuid: "myuuid"
  transaction_id: 8085312586421869529
}

------------------------------------------------------------
```
#### Help & Options
```
$ ./crud --help
usage: crud [-h] -n NODE {ping,create,read,update,delete,has,keys,size} ...

crud

positional arguments:
  {ping,create,read,update,delete,has,keys,size}
    ping                Ping
    create              Create k/v
    read                Read k/v
    update              Update k/v
    delete              Delete k/v
    has                 Determine whether a key exists within a DB by UUID
    keys                Get all keys for a DB by UUID
    size                Determine the size of the DB by UUID

optional arguments:
  -h, --help            show this help message and exit

required arguments:
  -n NODE, --node NODE  node's address (ex. 127.0.0.1:51010)
 ```
 
# SwarmDB

[![Build Status](https://travis-ci.org/bluzelle/swarmDB.svg?branch=devel)](https://travis-ci.org/bluzelle/swarmDB) [![License](https://img.shields.io/:license-AGPLv3-blue.svg?style=flat-square)](https://github.com/bluzelle/swarmDB/blob/devel/LICENSE) [![Twitter](https://img.shields.io/badge/twitter-@bluzelle-blue.svg?style=flat-square)](https://twitter.com/BluzelleHQ) [![Gitter chat](https://img.shields.io/gitter/room/nwjs/nw.js.svg?style=flat-square)](https://gitter.im/bluzelle)

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

## Building from source

### Dependencies - Minimum version requirements. Boost 1.67.0, gcc 7.1, cmake 3.12 and Protocol buffers 3.6.0.
Note, appropriate versions are in some cases provided by the system's package manager (apt, brew, yum, ect). In other cases, you might need to update your sources.lists to a test branch, or install the dependecies from source to obatin a supported version. 

### Linux Ubuntu 18.04
**Install the dependencies listed above**

### Clone devel branch
```text
$ git clone -b devel --single-branch https://github.com/bluzelle/swarmDB.git
$ cd swarmDB
$ git checkout devel
```

### Build and install
```text
$ mkdir build
$ cd build
$ cmake ..
$ sudo make install
```

### Other Linux distros

For the time being Ubuntu 18.04 is the only supported distro even though it will most likely build on others. If you receive this error while following the steps above on Debian

```text
make: *** No rule to make target 'install'.  Stop.
```

Edit the file swarmDB/pkg/CMakeLists.txt and comment out the two lines as per the example below and try again.

```text
if (UNIX AND NOT APPLE)
    find_program(LSB_RELEASE lsb_release)
    execute_process(COMMAND ${LSB_RELEASE} -is
        OUTPUT_VARIABLE LSB_RELEASE_ID_SHORT
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
  #  if (${LSB_RELEASE_ID_SHORT} STREQUAL "Ubuntu")
        add_subdirectory(debian)
  #  endif()
endif()

if (APPLE)
    add_subdirectory(osx)
endif()
```


### macOS

**Installing dependencies**

```text
$ brew update && brew install protobuf && brew install cmake && brew install gcc && brew install git
```

**ccache \(Optional\)**

If available, cmake will attempt to use ccache \([https://ccache.samba.org](https://ccache.samba.org)\) to _drastically_ speed up compilation.

```text
$ brew install ccache
```

### Clone repository 
```text
$ git clone -b devel --single-branch https://github.com/bluzelle/swarmDB.git
$ cd swarmDB
$ git checkout devel
```

### Build and install
```text
$ mkdir build
$ cd build
$ cmake ..
$ sudo make install
```

### Deploying the Daemon and configuration

#### The Bluzelle Configuration Files

The Bluzelle daemon is configured by setting the properties of two JSON
configuration files provided by the user. These files are called
*bluzelle.json* and *peers.json* and reside in the /output dir of the build directory. To specify a
different configuration file the daemon can be executed with the -c command
line argument:

    $ swarm -c path/to/config_file

The configuration files are JSON format files, as seen in the following example:

    {
        "bootstrap_file": "./peers.json",
        "ethereum": "0xddbd<...>121a",
        "ethereum_io_api_token": "53IW57FSZSZS3QXJUEBYT8F4YZ9IZFXBPQ",
        "listener_address": "127.0.0.1",
        "listener_port": 49152,
        "log_to_stdout": true,
        "uuid": "d6707510-8ac6-43c1-b9a5-160cf54c99f5",
        "max_storage" : "2GB",
        "logfile_dir" : "logs/",
        "logfile_rotation_size" : "64K"
        "logfile_max_size" : "640K",
        "debug_logging" : false
    }

Explaination of properties

- "bootstrap_file" - the path to a file containing the list of peers in the swarm that this node will be participating in. See below.
- "debug_logging" - set this value to true to include debug level log messages in the logs
- "ethereum" - is your Ethereum mainnet address.
- "ethereum_io_api_token" - this is used to identify the SwarmDB daemon to Etherscan Developer API (see https://etherscan.io/apis).
- "listener_address" - the ip address that SwarmDB will use
- "listener_port" - the socket address where SwarmDB will listen for protobuf and web socket requests.
- "log_to_stdout" - directs SwarmDB to log output to stdout when true.
- "logfile_dir" - location of log files (default: logs/)
- "logfile_max_size" - approx. maximum combined size of the logs before deletion occurs (default: 512K)
- "logfile_rotation_size" - approximate size of log file must be before rotation (default: 64K)
- "max_storage" - the approximate maximum limit for the storage that SwarmDB will use in the current instance (default: 2G)
- "uuid" - the universally unique identifier that this instance of SwarmDB will use to uniquely identify itself.

All size entries use the same notation as storage: B, K, M, G & T or none
(bytes)


#### The Bluzelle Bootstrap File

The bootstrap file, identified in the config file by the "bootstrap_file"
parameter, see above, provides a list of other nodes in the the swarm that the
local instance of the SwarmDB daemon can communicate with. Note that this may
not represent the current quorum so if it is sufficently out of date, you may
need to find the current list of nodes. The easiest way to get the most recent
list of nodes is to ask in gitter.im/bluzelle

The booststrap file format a JSON array, containing JSON objects describing
nodes as seen in the following example:

```
[{
  "name": "daemon01",
  "host": "13.78.131.94",
  "port": 51010,
  "uuid": "92b8bac6-3242-452a-9090-1aa48afd71a3",
  "http_port": 8080
}, {
  "name": "daemon02",
  "host": "13.78.131.94",
  "port": 51011,
  "uuid": "c864516b-3c95-4721-ad3e-a8dccd0d8349",
  "http_port": 8081
}, {
  "name": "daemon03",
  "host": "13.78.131.94",
  "port": 51012,
  "uuid": "137a8403-52ec-43b7-8083-91391d4c5e67",
  "http_port": 8082
},{
  "name": "your node,
  "host": "your public ip",
  "port": "your port (make sure this port is not blocked by a firewall or router)",
  "uuid": "your generated uuid",
  "http_port": 8083
  }]
```

where the Peer object parameters are:
- "name" - the human readable name that the external node uses
- "host" - the IP address associated with the external node
- "port" - the socket address that the external node will listen for protobuf and web socket requests.
- "uuid" - the universally unique identifier that the external node uses to uniquely identify itself.
- "http_port" - the HTTP port on which the external node listens for HTTP client requests.

Please ensure that a JSON object representing your node is also included
in the array of peers.


#### Requirements of the configuration files:

1. Placed in swarmDB/build/output/, where the swarm executable resides.
2. Valid Ethereum mainnet address with a balance > 0.
2. Valid Etherscan API KEY: [https://etherscan.io/register](https://etherscan.io/register)
    Create an Etherscan API KEY by clicking Developers -&gt; API-KEYs.
3. `listener_address` in bluzelle.json set to your local IP
4. `host`in peers.json set to your inet IP
5. Unique UUID for each daemon.
6. Listener address and port must match peer list.

**Adding your daemon to the network:**

Before you can spawn your daemon you need to add it to the network's leader's peers list. The leader is testnet-dev.bluzelle.com on port 51010, 51011, or 51012. Replace PORT below until you find the leader. The none leaders will return ERROR_ADD_PEER_MUST_BE_SENT_TO_LEADER, the leader return will not return anything.

```text
$ wscat -c testnet-dev.bluzelle.com:PORT
{"bzn-api":"raft","cmd":"add_peer","data":{"peer":{"host":"your_ip","http_port":8083,"name":"your_node_name","port":your_port,"uuid":"your_uuid"}}} 
```

**Deploying the daemon:**

From the directory where the executable resides.

```text
$ ./swarm -c bluzelle.json
```

## Integration Tests With Bluzelle's Javascript Client

### Installation - macOSX

#### Homebrew

```text
$ /usr/bin/ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"
```

#### Node

```text
$ brew install node
```

#### Yarn

```text
$ brew install yarn
```

### Installation - Ubuntu

#### NPM

```text
$ sudo apt-get install npm
```

#### Update NPM

```text
$ sudo npm install npm@latest -g
```

#### Yarn

```text
$ curl -sS https://dl.yarnpkg.com/debian/pubkey.gpg | sudo apt-key add -
echo "deb https://dl.yarnpkg.com/debian/ stable main" | sudo tee /etc/apt/sources.list.d/yarn.list

$ sudo apt-get update && sudo apt-get install yarn
```

### Running Integration Tests

Script to clone _bluzelle-js_ repository and copy template configuration files or run tests with your configuration files.

```text
$ qa/integration-tests setup // Sets up template configuration files
```

```text
$ qa/integration-tests // Runs tests with configuration files you've created
```

## Testing Locally

```text
$ cd scripts

Follow instructions in readme.md
```

#### Connectivity Test

```text
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

```text
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

```text
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

```text
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

```text
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

#### Adding or Removing A Peer

```text
Nodes are added to, or removed from, the network with the add_peer and
remove_peer commands, sent to a leader via WebSocket protocol with the
following JSON objects:

    Adding a peer:

    {
        "bzn-api":"raft",
        "cmd":"add_peer",
        "data":{
            "peer":{
                "host":"<HOST-URL>",
                "http_port":<HTTPPORT>,
                "name":"<NODE-NAME>",
                "port":<PORT>,
                "uuid":"<UUID>"
            }
        }
    }

    The "name" object, <NODE-NAME>, can be a human readable name for the node, "Fluffy" for example.
    The "uuid" object must be a universally unique identifer that uniquely identifies the node within the swarm, or any
    other swarm. This value can be generated online at a site like: https://www.uuidgenerator.net/

    Remove an existing peer:

    {
        "bzn-api":"raft",
        "cmd":"remove_peer",
        "data":{
            "uuid":"<UUID>"
        }
    }

Given a swarm of nodes, a new node can be added via the command line with a
WebSocket client such as wscat (https://www.npmjs.com/package/wscat).

Start the node that you want to add to the swarm, remember that the local peers
list must include the information for the local node for your node to be able
to start. When your node does start, it will not be able to participate in the
swarm.

Create your add_peer JSON object, and use wscat to send it to the swarm leader:

    $ wscat -c  http://<leader-address>:<port>
    connected (press CTRL+C to quit)
    >{"bzn-api":"raft","cmd":"add_peer","data":{"peer":{"host":"104.25.178.61","http_port":84,"name":"peer3","port":49154,"uuid":"7dda1fcb-d494-4fc1-8645-a14056d13afd"}}}
    >
    disconnected
    $

the new node will now start participating in the swarm.

To remove the node, create a remove_peer JSON object, and use wscat to send it
to the swarm leader:

    $ wscat -c  http://<leader-address>:<port>
    connected (press CTRL+C to quit)
    >{"bzn-api" : "raft", "cmd" : "remove_peer", "data" : { "uuid" : "7dda1fcb-d494-4fc1-8645-a14056d13afd" }}
    >
    disconnected
    $

and the node will be removed from the peer list.
```

#### Help & Options

```text
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


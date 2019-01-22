# SwarmDB

[![Build Status](https://travis-ci.org/bluzelle/swarmDB.svg?branch=devel)](https://travis-ci.org/bluzelle/swarmDB) [![Coverage Status](https://coveralls.io/repos/github/bluzelle/swarmDB/badge.svg?branch=devel)](https://coveralls.io/github/bluzelle/swarmDB?branch=devel) [![License](https://img.shields.io/:license-AGPLv3-blue.svg?style=flat-square)](https://github.com/bluzelle/swarmDB/blob/devel/LICENSE) [![Twitter](https://img.shields.io/badge/twitter-@bluzelle-blue.svg?style=flat-square)](https://twitter.com/BluzelleHQ) [![Gitter chat](https://img.shields.io/gitter/room/nwjs/nw.js.svg?style=flat-square)](https://gitter.im/bluzelle)

## ABOUT SWARMDB

Bluzelle brings together the sharing economy and token economy. Bluzelle enables people to rent out their computer storage space to earn a token while dApp developers pay with a token to have their data stored and managed in the most efficient way.

## Getting started with Docker

If you want to deploy your swarm immediately you can use our docker-compose quickstart instructions:

### Install Docker

[Docker Installation Guide](https://docs.docker.com/install/)

1. Setup a local docker-compose swarm with the instructions found [here](https://github.com/bluzelle/docker-swarm-deploy)
2. Run `docker-compose up` in the same directory of your docker-compose.yml. This command will initialize the swarm within your local docker-machine. Full docker-compose documentation can be found [here](https://docs.docker.com/compose/)
3. Nodes are available on localhost port 51010-51012
4. [Connect a test websocket client](https://github.com/bluzelle/swarmDB#testing-locally) 
5. Create a node server application using our node.js [library](https://github.com/bluzelle/bluzelle-js)
6. `CTRL-C` to terminate the docker-compose swarm

## Getting started building from source

### Installation - macOSX

**Dependencies \(Protobuf, CMake\)**

```text
$ brew update && brew install protobuf && brew install snappy && brew install lz4 && brew upgrade cmake
```

**ccache \(Optional\)**

If available, cmake will attempt to use ccache \([https://ccache.samba.org](https://ccache.samba.org)\) to _drastically_ speed up compilation.

```text
$ brew install ccache
```

### Installation - Ubuntu

**CMake**

```text
$ mkdir -p ~/mycmake
$ curl -L http://cmake.org/files/v3.11/cmake-3.11.0-Linux-x86_64.tar.gz | tar -xz -C ~/mycmake --strip-components=1
```

Again, this will result in a custom cmake install into `~/mycmake/` and will not overwrite your system's cmake.

**Protobuf \(Ver. 3 or greater\) etc.**

```text
$ sudo apt-get install pkg-config protobuf-compiler libprotobuf-dev libsnappy-dev
```

**ccache \(Optional\)**

If available, cmake will attempt to use ccache \([https://ccache.samba.org](https://ccache.samba.org)\) to _drastically_ speed up compilation.

```text
$ sudo apt-get install ccache
```

### Building the Daemon from Command Line Interface \(CLI\)

Here are the steps to build the Daemon and unit test application from the command line:

#### OSX

```text
$ mkdir build
$ cd build
$ cmake ..
$ sudo make install
```

#### Ubuntu

```text
$ mkdir build
$ cd build
$ ~/mycmake/bin/cmake ..
$ sudo make install
```

### Deploying the Daemons

#### The Bluzelle Configuration File

The Bluzelle daemon is configured by setting the properties of a JSON
configuration file provided by the user. This file is usually called
*bluzelle.json* and resides in the current working directory. To specify a
different configuration file the daemon can be executed with the -c command
line argument:

    $ swarm -c peer0.json

The configuration file is a JSON format file, as seen in the following example:

    {
				"listener_address" : "127.0.0.1",
				"listener_port" : 50000,
				"ethereum" : "0xddbd2b932c763ba5b1b7ae3b362eac3e8d40121a",
				"ethereum_io_api_token" : "<your API key here>",
				"bootstrap_file" : "/home/isabel/swarmdb/local/nodes/peers.json",
				"debug_logging" : true,
				"log_to_stdout" : true,
				"use_pbft": true,
        "bootstrap_file": "./peers.json",
    }

The complete documentation of the options available for this file is given by 

    $ swarm --help

but the properties likely useful for a minimal swarm are summarized here:

- "use_pbft" - use pbft consensus (instead of legacy raft code)
- "bootstrap_file" - path to peers file (see below)
- "debug_logging" - show more log info
- "ethereum" - is your Ethereum block chain address, used to pay for transactions. For now, you will not be charged, and you do not actually need to own this address.
- "ethereum_io_api_token" - this is used to identify the SwarmDB daemon to Etherscan Developer API (see https://etherscan.io/apis). Use the given value for now, this  property may be moved out the config file in the future.
- "listener_address" - the ip address that SwarmDB will listen on (this should be "127.0.0.1" unless you are doing something fancy)
- "listener_port" - the port that SwarmDB will listen on (each node running on the same host should use a different port)
- "log_to_stdout" (optional) - log to stdout as well as log file
- "uuid" - the universally unique identifier that this instance of SwarmDB will use to uniquely identify itself. This should be specified if and only if node cryptography is disabled (the default) - otherwise, nodes use their private keys as their identifier.

#### The Bluzelle Bootstrap File

The bootstrap file, identified in the config file by the "bootstrap_file"
parameter, see above, provides a list of nodes in the the swarm that the
local instance of the SwarmDB daemon can communicate with. If the membership
of the swarm has changed, these nodes will be used to introduce the node to
the current swarm and catch it up to the current state, and the bootstrap file
acts as a "starting peers list".

If you are running a static testnet (i.e., nodes do not join or leave the swarm)
then every node should have the same bootstrap_file, and it should include
an entry for every node. Thus, each node will appear in its own bootstrap file. 
If a node is not already in the swarm when it starts (i.e., it should dynamically join
the swarm) then it should not be in its own bootstrap file.

The booststrap file format is a JSON array, containing JSON objects describing
nodes as seen in the following example:

    [
        {
            "host": "127.0.0.1",
            "http_port": 9082,
            "name": "peer0",
            "port": 49152,
            "uuid": "d6707510-8ac6-43c1-b9a5-160cf54c99f5"
        },
        {
            "host": "127.0.0.1",
            "http_port": 9083,
            "name": "peer1",
            "port": 49153,
            "uuid": "5c63dfdc-e251-4b9c-8c36-404972c9b4ec"
        },
        ...
        {
            "host": "127.0.0.1",
            "http_port": 9083,
            "name": "peer1",
            "port": 49153,
            "uuid": "ce4bfdc-63c7-5b9d-1c37-567978e9b893a"
        }
    ]

where the Peer object parameters are (ALL PARAMETERS MUST MATCH THE PEER CONFIGURATION):
- "host" - the IP address associated with the external node 
- "http_port" - the HTTP port on which the external node listens for HTTP client requests.
- "name" - the human readable name that the external node uses
- "port" - the socket address that the external node will listen for protobuf and web socket requests. (listen_port in the config file)
- "uuid" - the universally unique identifier that the external node uses to uniquely identify itself. This is required to be unique per node and consistent between the peerlist and the config.

Note that if node cryptography is enabled (see swarmdb --help), node uuids are their public keys.


#### Steps to setup and run Daemon:

1. Create each of the JSON files as described above in swarmDB/build/output/, where the swarm executable resides. \(bluzelle.json, bluzelle2.json, bluzelle3.json, bluzelle4.json, peers.json\).
2. Create an account with Etherscan: [https://etherscan.io/register](https://etherscan.io/register)
3. Create an Etherscan API KEY by clicking Developers -&gt; API-KEYs.
4. Add your Etherscan API KEY Token to the configuration files.
5. Modify the `ethereum` address to be an Ethereum mainnet address that contains tokens or use the sample address provided above.
6. Ensure that each swarmdb instance is configured to listen on a different port and has a different uuid, and that the peers file contains correct uuids and addresses for all nodes
7. Deploy your swarm of Daemons. From the swarmDB/build/output/ directory, run:

```text
$ ./swarm -c bluzelle.json
$ ./swarm -c bluzelle2.json
$ ./swarm -c bluzelle3.json
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
$ ./crud -p -n localhost:50000 status

Client: crud-script-0
Sending: 
sender: "crud-script-0"
status_request: ""

------------------------------------------------------------

Response: 

swarm_version: "0.3.1443"
swarm_git_commit: "0.3.1096-41-g91cef89"
uptime: "1 days, 17 hours, 29 minutes"
module_status_json: ... 
pbft_enabled: true

Response: 
{
    "module" : 
    [
        {
            "name" : "pbft",
            "status" : 
            {
                "is_primary" : false,
                "latest_checkpoint" : 
                {
                    "hash" : "",
                    "sequence_number" : 3800
                },
                "latest_stable_checkpoint" : 
                {
                    "hash" : "",
                    "sequence_number" : 3800
                },
                "next_issued_sequence_number" : 1,
                "outstanding_operations_count" : 98,
                "peer_index" : 
                [
                    {
                        "host" : "127.0.0.1",
                        "name" : "node_0",
                        "port" : 50000,
                        "uuid" : "MFYwEAYHKoZIzj0CAQYFK4EEAAoDQgAE/HIPqL97zXbPN8CW609Dddu4vSKx/xnS1sle0FTgyzaDil1UmmQkrlTsQQqpU7N/kVMbAY+/la3Rawfw6VjVpA=="
                    },
                    {
                        "host" : "127.0.0.1",
                        "name" : "node_1",
                        "port" : 50001,
                        "uuid" : "MFYwEAYHKoZIzj0CAQYFK4EEAAoDQgAELUJ3AivScRn6sfBgBsBi3I18mpOC5NZ552ma0QTFSHVdPGj98OBMhxMkyKRI6UhAeuUTDf/mCFM5EqsSRelSQw=="
                    },
                    {
                        "host" : "127.0.0.1",
                        "name" : "node_2",
                        "port" : 50002,
                        "uuid" : "MFYwEAYHKoZIzj0CAQYFK4EEAAoDQgAEg+lS+GZNEOqhftj041jCjLabPrOxkkpTHSWgf6RNjyGKenwlsdYF9Xg1UH1FZCpNVkHhCLi2PZGk6EYMQDXqUg=="
                    }
                ],
                "primary" : 
                {
                    "host" : "127.0.0.1",
                    "host_port" : 50001,
                    "name" : "node_1",
                    "uuid" : "MFYwEAYHKoZIzj0CAQYFK4EEAAoDQgAELUJ3AivScRn6sfBgBsBi3I18mpOC5NZ552ma0QTFSHVdPGj98OBMhxMkyKRI6UhAeuUTDf/mCFM5EqsSRelSQw=="
                },
                "unstable_checkpoints_count" : 0,
                "view" : 1
            }
        }
    ]
}

------------------------------------------------------------
```

#### Create database
```text

./crud -p -n localhost:50000 create-db -u myuuid

Client: crud-script-0
------------------------------------------------------------

Response: 
header {
  db_uuid: "myuuid"
  nonce: 2998754133578549919
}

------------------------------------------------------------
```

#### Create

```text
$ ./crud -p -n localhost:50000 create -u myuuid -k mykey -v myvalue

Client: crud-script-0
------------------------------------------------------------
  
Response: 
header {
  db_uuid: "myuuid"
  nonce: 9167923913779064632
}
  
------------------------------------------------------------
```

#### Read

```text
$ ./crud -p -n localhost:50000 read -u myuuid -k mykey

Client: crud-script-0
------------------------------------------------------------

Response: 
header {
  db_uuid: "myuuid"
  nonce: 1298794800698891064
}
read {
  key: "mykey"
  value: "myvalue"
}

------------------------------------------------------------
```

#### Update

```text
$ ./crud -p -n localhost:50000 update -u myuuid -k mykey -v mynewvalue

Client: crud-script-0
------------------------------------------------------------

Response: 
header {
  db_uuid: "myuuid"
  nonce: 9006453024945657757
}

------------------------------------------------------------
```

#### Delete

```text
$ ./crud -p -n localhost:50000 delete -u myuuid -k mykey

Client: crud-script-0
------------------------------------------------------------

Response: 
header {
  db_uuid: "myuuid"
  nonce: 7190311901863172254
}

------------------------------------------------------------
```

#### Subscribe

```text
$ ./crud -p -n localhost:50000 subscribe -u myuuid -k mykey

Client: crud-script-0
------------------------------------------------------------

Response: 
header {
  db_uuid: "myuuid"
  nonce: 8777225851310409007
}

------------------------------------------------------------

Waiting....

Response: 
header {
  db_uuid: "myuuid"
  nonce: 8777225851310409007
}
subscription_update {
  key: "mykey"
  value: "myvalue"
}

------------------------------------------------------------

Waiting....
```

#### Delete database
```text
./crud -p -n localhost:50000 delete-db -u myuuid

Client: crud-script-0
------------------------------------------------------------

Response: 
header {
  db_uuid: "myuuid"
  nonce: 1540670102065057350
}

------------------------------------------------------------
```

#### Adding or Removing A Peer

Dynamically adding and removing peers is not supported in this release. This functionality will be available in a subsequent version of swarmDB.

#### Help & Options

```text
$ ./crud --help
usage: crud [-h] [-p] [-i ID] -n NODE
            {status,create-db,delete-db,has-db,writers,add-writer,remove-writer,create,read,update,delete,has,keys,size,subscribe}
            ...

crud

positional arguments:
  {status,create-db,delete-db,has-db,writers,add-writer,remove-writer,create,read,update,delete,has,keys,size,subscribe}
    status              Status
    create-db           Create database
    delete-db           Delete database
    has-db              Has database
    writers             Database writers
    add-writer          Add database writers
    remove-writer       Remove database writers
    create              Create k/v
    read                Read k/v
    update              Update k/v
    delete              Delete k/v
    has                 Determine whether a key exists within a DB by UUID
    keys                Get all keys for a DB by UUID
    size                Determine the size of the DB by UUID
    subscribe           Subscribe and monitor changes for a key

optional arguments:
  -h, --help            show this help message and exit
  -p, --use_pbft        Direct message to pbft instead of raft
  -i ID, --id ID        Crud script sender id (default 0)

required arguments:
  -n NODE, --node NODE  node's address (ex. 127.0.0.1:51010)

BUILD INSTRUCTIONS
==================

macOS BOOST Installation 
-
Bluzelle daemon uses the boost libraries, notably the Beast Library by Vinnie Falco, so we require Boost version 1.66.0, 
released on December 18th, 2017.    

NOTE: This will overwrite an existing Boost installation if it exists, if you have installed Boost with 
brew or some other package manager, you may wish to uninstall the older version of Boost first.

So open up a console and get started by downloading boost and building:
```
wget -c http://sourceforge.net/projects/boost/files/boost/1.66.0/boost_1_66_0.tar.bz2/download
tar jxvf download
rm download
cd boost_1_66_0
./bootstrap.sh 
./b2 toolset=darwin install
```

Feel free to use the "-j N" option with b2 to speed things up a bit -- you would need to determine how many CPU cores are on your machine. Basically, if you want to use multiple CPU cores, specify that number here in place of N. 

Now you will have the Boost 1.66.0 libraries installed in 

```/usr/local/lib```

and the include files in 

```/usr/local/include/boost```


*nix BOOST Installation
-
```
wget -c http://sourceforge.net/projects/boost/files/boost/1.66.0/boost_1_66_0.tar.bz2
tar jxf boost_1_66_0.tar.bz2
cd boost_1_66_0
sudo ./bootstrap.sh --prefix=/usr/local/
./b2
sudo ./b2 install 
```

Feel free to use the "-j N" option with b2 to speed things up a bit. If you want to use multiple CPU cores, specify that number here in place of N. Use the following command (on Linux), to get that number:

grep -c ^processor /proc/cpuinfo     

If you are getting errors that some Python headers are missing you need to install python-dev with and re-run b2 again
```
apt-get install python-dev
```

Windows BOOST Installation
-
TBD

CMake Installation
-
If you dont have CMake version 3.10 or above you have to install it. Please download it from https://cmake.org/download/
On Linux you can build CMake with (use -j option to build on multiple CPUs)
```
./bootstrap
make
sudo make install
```

Alternatively 
```
wget https://cmake.org/files/v3.10/cmake-3.10.1.tar.gz
tar zxvf cmake-3.10.1.tar.gz
cd cmake-3.10.1
./configure
make
sudo make install
```

Pre-built binaries also available for MacOS and Windows

NPM and WSCAT installation
-
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

CLONING THE REPO (All OS's)
-
We keep the project on github, so just clone the repo in a convenient folder:

git clone https://github.com/bluzelle/bluzelle.git

BUILDING THE DAEMON
-
If you have cLion, open the folder that you have just cloned and build and execute from there.

Otherwise, from most CLIs (OSX, Linux, etc), you can build from the command line. 
Here are the steps to build the daemon and unit test application from the command line:
```
git checkout devel
cd bluzelle
mkdir build
cd build
cmake ..
make
````

The executables ```the_db``` and ```the_db_test```  will be in the `daemon` folder.


SETTING ENVIRONMENT
-
- Go to https://etherscan.io/login?cmd=last and login (create an account if necessary). Click API-KEYs and then Create API Key and create a new token, this token will be used to make calls to Ethereum Ropsten network. Naming the token is optional.
- Create an environment variable named ETHERSCAN_IO_API_TOKEN with the newly created key as value (there are different ways to do that depending on system you are using). You can add it to your shell profile. Alternatively you can prepend the application you about to run with environment variable i.e.
```
ETHERSCAN_IO_API_TOKEN=TOKEN_GOES_HERE ./the_db --address 0x006eae72077449caca91078ef78552c0cd9bce8f --port 58001
```
- The API token is used to make a calls to the Ethereum network. In order to run a node the farmer is supposed to have at least 100 tokens in his/her account. The account address is provided as a parameter on command line. For the purpose of this demo we use the address 0x006eae72077449caca91078ef78552c0cd9bce8f that has the required amount. Currently the_db checks for the MK_13 token balance.

RUNNING THE APPLICATION
-
- Create 'peers' file in the same directory where the_db executable is located. Peers file contains the list of known nodes. Here is the example of peers file for 5 nodes running on localhost (';' can be used to comment lines):
```
; Comment
node_1=localhost:58001
node_2=localhost:58002
node_3=localhost:58003
node_4=localhost:58004
node_5=localhost:58005
```
- Create a simple shell script to start multiple nodes. 
- For Ubuntu Linux:
```
#!/bin/bash

gnome-terminal -x bash -c './the_db --address 0x006eae72077449caca91078ef78552c0cd9bce8f --port 58001'
gnome-terminal -x bash -c './the_db --address 0x006eae72077449caca91078ef78552c0cd9bce8f --port 58002'
gnome-terminal -x bash -c './the_db --address 0x006eae72077449caca91078ef78552c0cd9bce8f --port 58003'
gnome-terminal -x bash -c './the_db --address 0x006eae72077449caca91078ef78552c0cd9bce8f --port 58004'
gnome-terminal -x bash -c './the_db --address 0x006eae72077449caca91078ef78552c0cd9bce8f --port 58005'
```
- For GNU Screen:
```
#!/bin/bash
screen -dmS bluzelle_nodes; 
screen -S bluzelle_nodes -X screen ./the_db --address 0x006eae72077449caca91078ef78552c0cd9bce8f --port 58001;
screen -S bluzelle_nodes -X screen ./the_db --address 0x006eae72077449caca91078ef78552c0cd9bce8f --port 58002;
screen -S bluzelle_nodes -X screen ./the_db --address 0x006eae72077449caca91078ef78552c0cd9bce8f --port 58003;
screen -S bluzelle_nodes -X screen ./the_db --address 0x006eae72077449caca91078ef78552c0cd9bce8f --port 58004;
screen -S bluzelle_nodes -X screen ./the_db --address 0x006eae72077449caca91078ef78552c0cd9bce8f --port 58005;
```
- For tmux:
```
#!/bin/bash
tmux new-session -d -s bluzelle_nodes
tmux new-window -t bluzelle_nodes:1 './the_db --address 0x006eae72077449caca91078ef78552c0cd9bce8f --port 58001';
tmux new-window -t bluzelle_nodes:2 './the_db --address 0x006eae72077449caca91078ef78552c0cd9bce8f --port 58002';
tmux new-window -t bluzelle_nodes:3 './the_db --address 0x006eae72077449caca91078ef78552c0cd9bce8f --port 58003';
tmux new-window -t bluzelle_nodes:4 './the_db --address 0x006eae72077449caca91078ef78552c0cd9bce8f --port 58004';
tmux new-window -t bluzelle_nodes:5 './the_db --address 0x006eae72077449caca91078ef78552c0cd9bce8f --port 58005';
```
- on macOS:
You can either open terminal.app and from the directory where you have the_db, paste the line ./the_db --address 0x006eae72077449caca91078ef78552c0cd9bce8f --port 58001, open a new tab (cmd-t) and paste the following line, ./the_db --address 0x006eae72077449caca91078ef78552c0cd9bce8f --port 58002, repeat with ports 58003, 58005 and 58005. Or you can install gnu screen or tmux with homebrew and run the scripts for them.

Name the script run.sh and put into the same directory where the_db is located. Use ```sudo chmod +x ./run.sh``` to make it executable from command line. Each line in script is responsible for running an instance of the_db. Arguments are --address - Ethereum address on Ropsten network for account that has a MK_13 token balance higher that 100 (you can use known address 0x006eae72077449caca91078ef78552c0cd9bce8f) and --port - port the_db listens on. Port must be in range 49152 - 65535 and unique for each instance of the_db.
- From the directory where you have your ```peers``` file, ```run.sh``` file and ```the_db``` open terminal window and run script.
```
./run.sh
```
The script will start and spawn 5 terminal windows each running an instance of the_db. Instances use RAFT protocol (check out interactive demo put together by creator of RAFT http://thesecretlivesofdata.com/raft/) to elect a leader node that will be responsible for consistency of data. At random interval each node will nominate itself for a leader and will vote if it receives a vote request from another node. If node haven't nominated itself yet it will vote 'YES' otherwise it will vote 'NO'. The node that starts election first receives votes first and if it has enough 'YES' votes it will become the swarm leader and start sending heartbeat messages to all known nodes (nodes that are in ```peers```file). When node receives heartbeat from the leader it becomes a follower.
- Now we can test data replication. Check all the_db instances and find out the address/port of the leader. Create _anoter_ terminal window and create ```wscat``` session to the leader (this window will emulate API) i.e.:
```
wscat -c localhost:58002
```
Actual address/port of the leader will be different if you have a different settings in your ```peers``` file.
- Paste following JSON to ```wscat``` and press Enter:
```
{"bzn-api":"create", "transaction-id":"123", "data":{"key":"key_222", "value":"value_222"}}
```
It will send command to leader to create an entry in database with key ```key_222``` and value ```value_222``` (feel free to use any key/values). Transaction ID is arbitrary for now.
Run ```wscat``` again and connect to one of the followers nodes (running on ports 58001 to 58005).
```
wscat -c localhost:58004
```
- Paste following JSON and press Enter.
```
{"bzn-api":"read", "transaction-id":"123", "data":{"key":"key_222"}}
```
This will send request to the node to retrieve value for key ```key_222```. You should get JSON back with result ```value_222```.
- You can disconnect and connect to another follower node and send same JSON, correct value will be returned.
The data was successfully propagated from leader to followers nodes.
- Now we can go to the leader instance and stop it with Ctrl-C (remember leader IP/port).
- Nodes will stop receiving heartbeat message from the leader and will wait for some time before entering in to the Candidate mode again and starting new election round.
- Open another terminal session and change directory to where the_db is located.  Start the_db again (oyou must use the same port and ip that is in you ```peers``` file. Just be sure that two instances don't use the same ip/port).
- The instance you just started will start in Candidate mode and will wait for some time to receive a message from the leader. If it receives the heartbeat it will join existing swarm. If election has not started yet it could start a new round and become the leader again. The end result is that node joins existing swarm as the follower or the leader.
- Check all instances and find out who is the leader again. You can create another ```wscat``` session to the leader and try CRUD command again and see how data propagated to followers.
- You can retrieve previously stored data from any of the followers instances except the instance that was shutdown and started again (former leader). There is no persistence for the database implemented yet so data does not survive restart.

The above demonstrate basic RAFT functionality and ability to create a swarm and ability of nodes to leave/join existing swarm. Also data consistency is ensured across the nodes of the network (will be fully consistent when persistence is implemented). 


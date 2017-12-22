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
tar -xzvf download
rm download
cd boost_1_66_0
./bootstrap.sh 
./b2 toolset=darwin install
```

Feel free to use the "-j" option with b2 to speed things up a bit. 

Now you will have the Boost 1.66.0 libraries installed in 

```/usr/local/lib```

and the include files in 

```/usr/local/include/boost```


*nix BOOST Installation
-
TBD



Windows BOOST Installation
-
TBD

CLONING THE REPO (All OS's)
-
We keep the project on github, so just clone the repo in a convienient folder:

git clone https://github.com/bluzelle/bluzelle.git

BUILDING THE DAEMON
-
If you have cLion, open the folder that you have just cloned otherwise you can build from the commandline. 

We use cmake (http://cmake.org) to generate the make files that build the project. Make sure that you have at least cmake 
version 3.10.0(the Bluzelle dev team uses that one).

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
- get wscat ```npm install wscat -g```
- Go to https://etherscan.io and create an account. In your accout profile go to My API Keys section and create a new token, this token will be used to make calls to Ethereum Ropsten network. 
- Create environment variable named ETHERSCAN_IO_API_TOKEN with newly created key as value (there are differnt ways to do that depending on which system you are using)

RUNNING THE APPLICATIONS
-
- Change to the directory where the_db file is located and open 6 terminal sessions (or tabs).
- In first 5 tabs run the_db in "follower" mode i.e. using port number that doesn't start with 0. for example:
```./the_db --address 0x006eae72077449caca91078ef78552c0cd9bce8f --port 58001```
```./the_db --address 0x006eae72077449caca91078ef78552c0cd9bce8f --port 58002```
```./the_db --address 0x006eae72077449caca91078ef78552c0cd9bce8f --port 58003```
```./the_db --address 0x006eae72077449caca91078ef78552c0cd9bce8f --port 58004```
```./the_db --address 0x006eae72077449caca91078ef78552c0cd9bce8f --port 58005```
- Then in remaining tab run the_db in leader mode i.e. with port ending with 0. The leader will start sending heartbeats to followers.
```./the_db --address 0x006eae72077449caca91078ef78552c0cd9bce8f --port 58000```
- Open _another_terminal window. This window will emulate API. Type 
```'wscat -c localhost:58000'```
It will create websocket connection to the leader node (running on port 58000)
- Paste following JSON to wscat and press Enter:
```{"bzn-api":"create", "transaction-id":"123", "data":{"key":"key_222", "value":"value_222"}}```
It will send command to leader to create an entry in database with key ```key_222``` and value ```value_222``` (feel free to use any key/values).
- Press Ctrl-C to disconnect from leader node.
Run wscat again and connect to one of the followers nodes (running on ports 58001 to 58005).
```wscat -c localhost:58004```
- Paste following JSON and press Enter.
```{"bzn-api":"read", "transaction-id":"123", "data":{"key":"key_222"}}```
This will send request to the node to retrieve value for key ```key_222```. You should get JSON back wit result ```value_222```.
- You can disconnect and connect to another follower node and send same JSON, correct value will be returned.

This means that data was successfully propagated from leader to followers nodes.


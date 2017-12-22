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


RUNNING THE APPLICATIONS
=
- get wscat ```npm install wscat -g```
- https://etherscan.io and get an ETHERSCAN_IO_API_TOKEN
...
TODO: Dmitry, please wrtie the instructions for running the demo here.

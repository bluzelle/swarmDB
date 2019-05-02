swarmDB uses several precompiled libraries for faster CI builds.

##### Set the required library build number in depend/CMakelists:
```text  
    ...
    
    add_external_project(boost 1.70.0)
    add_external_project(googletest 1.8.0)
    add_external_project(jsoncpp 1.8.4)
    add_external_project(openssl 1.1.1)
    add_external_project(rocksdb 5.14.3)
```    
##### To add the debug version of Boost 1.70.0 using git LFS:
###### For release use: -DCMAKE_BUILD_TYPE=Release
```text
$ cd depend/boost/package
$ mkdir build-debug && cd build-debug
$ cmake .. -DCMAKE_BUILD_TYPE=Debug -DPKG_VER=1.70.0 [-DPKG_HASH=<sha256>]
 ```

 ###### PKG_HASH is optional and if given will be used to validate the downloaded source code archive.   
 ```text
-- The CXX compiler identification is GNU 8.3.0
-- Check for working CXX compiler: /usr/bin/c++
-- Check for working CXX compiler: /usr/bin/c++ -- works
-- Detecting CXX compiler ABI info
-- Detecting CXX compiler ABI info - done
-- Detecting CXX compile features
-- Detecting CXX compile features - done
-- boost -- debug 1.70.0 (chrono,program_options,random,regex,system,thread,log,serialization)
-- boost -- URL: https://dl.bintray.com/boostorg/release/1.70.0/source/boost_1_70_0.tar.gz
-- boost -- URL_HASH: 882b48708d211a5f48e60b0124cf5863c1534cd544ecd0664bb534a4b5d506e9
-- Configuring done
-- Generating done
-- Build files have been written to: /home/username/bluzelle/swarmdb/depend/boost/package/build-debug

$ make
Scanning dependencies of target boost
[ 12%] Creating directories for 'boost'
[ 25%] Performing download step (download, verify and extract) for 'boost'
-- Downloading...
   dst='/home/username/bluzelle/swarmdb/depend/boost/package/build-debug/boost/src/boost_1_70_0.tar.gz'
   timeout='120 seconds'
-- Using src='https://dl.bintray.com/boostorg/release/1.70.0/source/boost_1_70_0.tar.gz'
-- verifying file...
       file='/home/username/bluzelle/swarmdb/depend/boost/package/build-debug/boost/src/boost_1_70_0.tar.gz'
-- Downloading... done
-- extracting...
     src='/home/username/bluzelle/swarmdb/depend/boost/package/build-debug/boost/src/boost_1_70_0.tar.gz'
     dst='/home/username/bluzelle/swarmdb/depend/boost/package/build-debug/boost/src/boost'
-- extracting... [tar xfz]
-- extracting... [analysis]
-- extracting... [rename]
-- extracting... [clean up]
-- extracting... done
    ...   
[ 87%] No install step for 'boost'
[100%] Completed 'boost'
[100%] Built target boost
$
```

##### Create a package to be added to the repo
```text
$ make package
[100%] Built target boost
Run CPack packaging tool...
CPack: Create package using TGZ
CPack: Install projects
CPack: - Run preinstall target for: boost
CPack: - Install project: boost
CPack: Create package
CPack: - package: /home/username/bluzelle/swarmdb/depend/boost/package/build-debug/boost-debug-1.70.0-linux.tar.gz generated.
$ mv boost-debug-1.70.0-linux.tar.gz ..
$ cd ..
$ git lfs track boost-debug-1.70.0-linux.tar.gz
$ git add .gitattributes boost-debug-1.70.0-linux.tar.gz
$ git commit -m "added debug Boost 1.70.0 for Linux"
```

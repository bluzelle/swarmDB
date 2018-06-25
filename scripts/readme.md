#Python CRUD Test App

Steps to run the Bluzelle SwarmDB test application in a Python Virtual Environment. **Ensure that you activate your virtualenv each time you want to run the application**.


###Install Virtual Environment


    $ sudo pip install virtualenv
    $ cd workspace
    $ virtualenv crud-app

### Activate Virtual Environment
    $ . ~/workspace/crud-app/bin/activate
    
### Install Dependencies

    (crud-app)$ pip install websocket-client protobuf

### Generate Protobuf Code
    (crud-app)$ cd ../workspace/swarmdb/scripts
    (crud-app)$ protoc --python_out=../scripts ./bluzelle.proto ./database.proto
    
### Run Script
    (crud-app)$ crud -h
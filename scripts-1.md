# Python CRUD Test App

Steps to run the Bluzelle SwarmDB test application in a Python Virtual Environment. **Ensure that you activate your virtualenv each time you want to run the application**.

## Install Virtual Environment

```text
$ sudo pip install virtualenv
$ cd workspace
$ virtualenv crud-app
```

## Activate Virtual Environment

```text
$ . ~/workspace/crud-app/bin/activate
```

## Install Dependencies

```text
(crud-app)$ pip install websocket-client protobuf
```

## Generate Protobuf Code

```text
(crud-app)$ cd ../workspace/swarmdb/scripts
(crud-app)$ protoc --python_out=../scripts *.proto
```

## Run Script

```text
(crud-app)$ crud -h
```


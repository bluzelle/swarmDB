#!/usr/bin/python
import websocket
import json
import base64
import sys

def base_req(request_id, user_id, cmd):
    req = {
        "bzn-api" : "crud",
        "db_uuid" : user_id,
        "cmd" : cmd,
        "request-id" : request_id
    }
    return req

def create_req(request_id, user_id, key, value):
    req = base_req(request_id, user_id, "create" )
    value = str(base64.b64encode(value.encode("utf-8")))
    req["data"] = {
        "key" : key,
        "value" : value
    }
    return json.dumps(req)

def read_req(request_id, user_id, key):
    req = base_req(request_id, user_id, "read")
    req["data"] = {
        "key" : key
    }
    return json.dumps(req)

def update_req(request_id, user_id, key, value):
    req = base_req(request_id, user_id, "update")
    value = str(base64.b64encode(value.encode("utf-8")))
    req["data"] = {
        "key" : key,
        "value" : value
    }
    return req

def delete_req(request_id, user_id, key):
    req = base_req(request_id, user_id, "delete")
    req["data"] = {"key":key}
    return req


def send_request(ws, port, req):
    ws.connect("ws://127.0.0.1:" + str(port))
    ws.send(req)



#ws = websocket.WebSocket()

print(create_req(0, "me", "key", "value"))
print(read_req(1, "me", "key"))
print(update_req(2, "me", "key", "value0"))
print(delete_req(3, "me", "key"))




#!/usr/bin/python
import websocket
import sys

ws = websocket.WebSocket()
for x in range(0,10):
    ws.connect("ws://127.0.0.1:49152")
    ws.send( '{"bzn-api":"crud" , "data" : ' + str(x) + '}' )

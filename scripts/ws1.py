#!/usr/bin/python
import websocket
import sys

ws = websocket.WebSocket()
ws.connect("ws://127.0.0.1:49152")

for x in range(0,20):   
    ws.send( '{"bzn-api":"ping" , "data" : ' + str(x) + '}' )
    print ws.recv()

ws.close()

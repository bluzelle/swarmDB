CODE LAYOUT
===========
- folders are features (no src/inc)

MVP
===
- executable (Dmitry,Rich) called the_db
- parameters etherium address, port, (--addr=<addr> --port=<port>) (Dmitry)
- ws (Rich)
- push model:someone connects to the node, the node pushes messages (Dmitry)
- services: node info daemon sends this to gui on ws connection
{ 
    "cmd":"updateNodes", 
    "seq":123, 
    "data":[
       { 
       "address":"123.22.22.22",
       "name":"fluffy_kitty",
       "isLeader":true,
       "available":2128,
       "used":228
       }]
}
- nodes know about each other via ini files: ini file in ~/.bluzelle/.keplerrc (Mehdi)


QUESTIONS
=========


LATER
=====

- wss
- https
- nodes can discover friends of nodes and add those addresses to their ini.
- nodes will connect
- nodes will do raft
- update nodes for the nodes the node knows about
- get peers
{"cmd":"getPeers", "seq":132}
{"response":"getPeers", "peers": [{"123.22.33.33", "fluffy_bunny"},{"232.22.33.22", "fluffy_puppy"}], "seq":132}
- update nodes 

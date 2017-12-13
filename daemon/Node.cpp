#include <iostream>

#include <boost/lexical_cast.hpp>

#include "Node.h"


Node::Node(boost::asio::io_service& ios, const NodeInfo& i)
        : ios_(ios),
          raft_(ios_, i),
          server_(ios_,
                  "127.0.0.1", // [todo] localhost?
                  i.port_,
                  1,
                  // Raft::handle_request() is processing JSON.
                  std::bind(&Raft::handle_request,
                            &raft_,
                            std::placeholders::_1)) {

}

void Node::run() {
    raft_.run(); // Start RAFT.
    server_.run(); // Start accepting connections.

    ios_.run();

    std::cout << "Press a key to stop" << std::endl;
    getchar();
    std::cout << "Stopping..." << std::endl;
}

vector<NodeInfo> Node::get_peers() const {
    vector<NodeInfo> v;
    // [todo] populate nodes vector.
    return v;
}

vector<MessageInfo> Node::get_messages() const {
    vector<MessageInfo> v;
    // [todo] populate messages vector.
    return v;
}
#include "Node.h"
#include "NodeUtilities.h"
#include "web_sockets/WebSocketServer.h"
#include "web_sockets/WebSocket.h"
#include "web_sockets/Listener.h"
#include "web_sockets/Session.h"
#include "server/server.hpp"
#include "NodeManager.h"

#include <boost/exception/all.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/config.hpp>
#include <iostream>
#include <string>

static unsigned long s_max_nodes = 25;
static unsigned long s_min_nodes = 5;
static Nodes s_nodes;
static boost::mutex *s_mutex = nullptr;

// List of nodes that were died and need to be reported to GUI.
static Nodes s_removed_nodes;
static boost::mutex s_removed_nodes_mutex;
Nodes* get_removed_nodes() { return &s_removed_nodes; }
boost::mutex& get_removed_nodes_mutex() { return s_removed_nodes_mutex; }

std::shared_ptr<Listener> g_listener; // Listener object from WebSocketServer.


void print_message(const std::string &msg);

void kill_and_join_all_nodes(Nodes &nodes);

void make_introductions(const Nodes &nodes);

boost::mutex &get_mutex();

Nodes create_nodes(int number_of_nodes, std::shared_ptr<Listener> l);


void add_nodes(const Nodes &nodes)
{
    boost::mutex::scoped_lock lock(*s_mutex);
    s_nodes.insert(s_nodes.end(), nodes.begin(), nodes.end());
}

void set_max_nodes(unsigned long max)
{
    s_max_nodes = max;
}

unsigned long get_max_nodes()
{
    return s_max_nodes;
}

unsigned long get_min_nodes()
{
    return s_min_nodes;
}

void http_service()
{
    const std::string address("127.0.0.1");
    const std::string port("2999");
    std::string const doc_root = "../web/";
    http::server::server s(address, port, doc_root);
    s.run();
}

int main(/*int argc,char *argv[]*/) {
    uint8_t numTasks = 2;
    get_mutex();

    boost::thread websocket_thread(WebSocketServer("127.0.0.1", 3000, 1));
    boost::thread http_thread(http_service);

    std::stringstream ss;
    try
        {
        s_nodes = create_nodes(numTasks, g_listener);
        wait_for_all_nodes_to_start(s_nodes);
        make_introductions(s_nodes);

        while (!s_nodes.empty())
            {
            reaper(&s_nodes);
            int num_of_new_nodes = number_of_nodes_to_create(s_min_nodes, s_max_nodes, s_nodes.size());

            if (num_of_new_nodes > 0)
                {
                Nodes nodes = create_nodes(num_of_new_nodes, g_listener);
                add_nodes(nodes);
                }
            }

        // clean up
        print_message("The app is ending\n");
        kill_and_join_all_nodes(s_nodes);
        }

    catch (const std::exception &e)
        {
        std::cout << "Caught exception: [" << e.what() << "]\n";
        }

    catch (...)
        {
        std::cout << "caught exception\n";
        }

    // clean up
    http_thread.join();
    websocket_thread.join();


    return 0;
}

/////////


Nodes *get_all_nodes()
{
    return &s_nodes;
}

Node *create_node(std::shared_ptr<Listener> l)
{
    return new Node(l);
}

Nodes create_nodes(int number_of_nodes, std::shared_ptr<Listener> l)
{
    Nodes nodes;
    for (int i = 0; i < number_of_nodes; ++i)
        {
        nodes.emplace_back(create_node(l));
        }
    return nodes;
}

void kill_nodes(const Nodes &nodes)
{
    for (auto node : nodes)
        {
        node->kill();
        }
}

void join_node_threads(const Nodes &nodes)
{
    for (auto node : nodes)
        {
        node->join();
        }
}

void kill_and_join_all_nodes(Nodes &nodes)
{
    kill_nodes(nodes);
    join_node_threads(nodes);
}

void make_introductions(const Nodes &nodes)
{
    Node *last_node = nullptr;
    for (auto node : nodes)
        {
        if (nullptr != last_node)
            {
            last_node->ping(node->get_thread_id());
            }
        last_node = node;
        }
}

boost::mutex &get_mutex()
{
    if (nullptr == s_mutex)
        {
        s_mutex = new boost::mutex();
        }
    return *s_mutex;
}

void print_message(const std::string &msg)
{
    //boost::unique_lock<boost::mutex> guard(*s_mutex, boost::defer_lock);
    //guard.lock();
    std::cout << msg;
    //guard.unlock();
}

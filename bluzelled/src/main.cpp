#include "NodeUtilities.h"
#include "web_sockets/WebSocketServer.h"
#include "http/server/ServiceManager.h"
#include "NodeManager.h"

#include <cstdio>
#include <boost/exception/all.hpp>
#include <boost/beast/http.hpp>

void initialize
        (
        std::shared_ptr<Listener> &listener,
        std::shared_ptr<NodeManager> &node_manager,
        std::shared_ptr<boost::thread> &websocket_thread,
        std::shared_ptr<http::server::ServiceManager> &http_manager
        )
{
    node_manager = std::make_shared<NodeManager>(1);
    // TODO: get websocket and service manager args from command line args.
    websocket_thread = std::make_shared<boost::thread>(WebSocketServer("127.0.0.1", 3000, 1, node_manager, listener));
    http_manager = std::make_shared<http::server::ServiceManager>("127.0.0.1", "2999", "../web/");
    http_manager->run_async();
    node_manager->create_and_add_nodes(listener, 1);
    wait_for_all_nodes_to_start(node_manager->nodes());
}

void run
        (
                std::shared_ptr<Listener> &listener,
                std::shared_ptr<NodeManager> &node_manager
        )
{
    while (!node_manager->nodes().empty())
        {
        reaper(&node_manager->nodes());
        size_t num_of_new_nodes = number_of_nodes_to_create(
                node_manager->min_nodes(),
                node_manager->max_nodes(),
                node_manager->nodes_count());

        if (num_of_new_nodes > 0)
            {
            node_manager->create_and_add_nodes(listener, num_of_new_nodes);
            }
        }

}

void clean_up
        (
                std::shared_ptr<NodeManager> &node_manager,
                std::shared_ptr<boost::thread> &websocket_thread
        )
{
    node_manager->kill_all_nodes();
    node_manager->join_all_threads();
    websocket_thread->join();
}


int main(/*int argc,char *argv[]*/) {
    try
        {
        std::shared_ptr<Listener> listener;
        std::shared_ptr<NodeManager> node_manager;
        std::shared_ptr<boost::thread> websocket_thread;
        std::shared_ptr<http::server::ServiceManager> http_manager;

        initialize(listener, node_manager, websocket_thread, http_manager);

        run
                (
                        listener,
                        node_manager
                );

        clean_up
                (
                        node_manager,
                        websocket_thread
                );
        }

    catch (const std::exception &e)
        {
        std::cout << "Caught exception: [" << e.what() << "]\n";
        }

    catch (...)
        {
        std::puts("Caught anonymous exception.");
        }

    std::puts("Goodbye. Thank you for using TheDB Simulator!");
    return 0;
}

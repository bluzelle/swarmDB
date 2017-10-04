#include "NodeUtilities.h"

#include <iostream>

bool all_nodes_alive(const Nodes& nodes)
{
    for(auto node : nodes)
        {
        if(node->state() != Task::alive)
            {
            return false;
            }
        }
    return !nodes.empty();
}

void wait_for_all_nodes_to_start(const Nodes &nodes)
{
    while (!all_nodes_alive(nodes))
        {
        boost::this_thread::sleep(boost::posix_time::milliseconds(100));
        }
}

void reaper(Nodes *nodes)
{
    for(auto node : *nodes)
        {
        if(node->state() == Task::dead)
            {
            if(node->is_joinable())
                {
                node->join();
                }
            }
        }


    std::cout << "**** before erase\n";
    nodes->erase
            (
                    std::remove_if
                            (
                                    nodes->begin(),
                                    nodes->end(),
                                    [](auto node)
                                        {
                                        return (node->state() == Task::dead);
                                        }
                            )
            );

    std::cout << " ***** nodes:[" << nodes->size() << "]\n\n";

}


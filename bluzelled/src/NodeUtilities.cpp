#include "NodeUtilities.h"

#include <algorithm>
#include <iostream>

Nodes* get_removed_nodes();
boost::mutex& get_removed_nodes_mutex();

boost::mutex& get_mutex();


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

void join_dead_tasks(Nodes *nodes)
{
    for(auto n : *nodes )
        {
        if(n->state() == Task::dead)
            {
            n->join();
            }
        }
}

void remove_dead_nodes(Nodes *nodes)
{
    auto is_task_dead = [](Node* n) {return (n->state() != Task::dead);};

    auto it = std::stable_partition(nodes->begin(), nodes->end(), is_task_dead);
    //rnodes->insert(rnodes->end(), std::make_move_iterator(it), std::make_move_iterator(nodes->end()));

    nodes->erase(it, nodes->end());

    //if (rnodes->size() > 0)
    //    std::cout << "dead nodes: " << rnodes->size() << std::endl;
}


void reaper(Nodes *nodes)
{
    join_dead_tasks(nodes);
    remove_dead_nodes(nodes);
}

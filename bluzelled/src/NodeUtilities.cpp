#include "NodeUtilities.h"

#include <string>

bool all_nodes_alive(const Nodes &nodes)
{
    for (auto node : nodes)
        {
        if (node->state() != Task::alive)
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
    for (auto n : *nodes)
        {
        if (n->state() == Task::dead)
            {
            n->join();
            }
        }
}

void remove_dead_nodes(Nodes *nodes)
{
    auto is_task_dead = [](Node *n)
        { return (n->state() != Task::dead); };

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

std::string generate_ip_address()
{
    static boost::random::mt19937 rng(time(0));

    boost::random::uniform_int_distribution<> two_two_five(0, 255);

    int ip[4] = {0};
    for (auto &addr : ip)
        {
        addr = two_two_five(rng);
        }

    auto f = boost::format("%d.%d.%d.%d") % ip[0] % ip[1] % ip[2] % ip[3];
    return boost::str(f);
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

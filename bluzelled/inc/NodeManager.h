#ifndef KEPLER_NODEMANAGER_H
#define KEPLER_NODEMANAGER_H

#include <web_sockets/Listener.h>
#include "Node.h"

#include <cstddef>
#include <exception>
#include <algorithm>

#define kMINIMUM_ERROR "The minimum number of nodes must be smaller than the maximum number of nodes."
#define kMAXIMUM_ERROR "The maximum number of nodes must be larger than the minimum number of nodes."

class NodeManager
{
    size_t min_nodes_;
    size_t max_nodes_;
    std::vector<Node *> nodes_;

public:
    NodeManager
            (
                    size_t min = 5, size_t max = 25
            ) : min_nodes_(min), max_nodes_(max)
    {
        nodes_.reserve(max_nodes());
    }

    size_t min_nodes()
    {
        return min_nodes_;
    }

    void set_min_nodes(const size_t min)
    {
        if (min < max_nodes())
            {
            min_nodes_ = min;
            }
        else
            {
            throw std::runtime_error(kMINIMUM_ERROR);
            }

    }

    void set_max_nodes(const size_t &max)
    {
        if (max > min_nodes())
            {
            max_nodes_ = max;
            nodes_.reserve(max);
            }
        else
            {
            throw std::runtime_error(kMAXIMUM_ERROR);
            }
    }

    size_t max_nodes()
    {
        return max_nodes_;
    }

    size_t nodes_count()
    {
        return nodes_.size();
    }

    Nodes &nodes()
    {
        return this->nodes_;
    }

    void kill_all_nodes()
    {
        std::for_each
                (
                        nodes_.begin()
                        , nodes_.end()
                        , [](auto& n){ n->kill(); }
                );
    }

    void join_all_threads()
    {
        std::for_each
                (
                        nodes_.begin()
                        , nodes_.end()
                        , [](auto& n){ n->join(); }
                );
    }

    bool all_nodes_alive()
    {
        for (auto node : nodes_)
            {
            if (node->state() != Task::alive)
                {
                return false;
                }
            }
        return !nodes_.empty();
    }

    void create_and_add_nodes
            (
                    const std::shared_ptr<Listener> &l
                    , const size_t num_of_new_nodes)
    {
        Nodes new_nodes;
        new_nodes.reserve(num_of_new_nodes);
        for(size_t i=0; i<num_of_new_nodes; ++i)
            {
            new_nodes.push_back(new Node(l));
            }
        nodes_.insert( nodes_.end(), new_nodes.begin(), new_nodes.end() );
    }

private:

    Node *create_node(const std::shared_ptr<Listener> &listener)
    {
        return new Node(listener);
    }

    Nodes create_nodes(const std::shared_ptr<Listener> &listener, size_t number_of_nodes)
    {
        Nodes nodes;
        nodes.reserve(number_of_nodes);
        std::generate(nodes.begin(), nodes.end(),
                      [this, listener]()
                          {
                          return create_node(listener);
                          });


        return nodes;
    }

    void add_nodes()
    {}

    void clean_up()
    {}

    void reaper()
    {
        join_dead_tasks();
        remove_dead_nodes();
    }

    void join_dead_tasks()
    {
        for (auto n : nodes_)
            {
            if (n->state() == Task::dead)
                {
                n->join();
                }
            }
    }

    void remove_dead_nodes()
    {
        auto is_task_dead = [](auto n)
            { return (n->state() == Task::dead); };
        auto r = std::remove_if
                (
                        nodes_.begin(),
                        nodes_.end(),
                        is_task_dead
                );
        if (r != nodes_.end())
            {
            nodes_.erase(r, nodes_.end());
            }
    }


};

unsigned long
number_of_nodes_to_create(unsigned long min_nodes, unsigned long max_nodes, unsigned current_number_of_nodes);

#endif //KEPLER_NODEMANAGER_H

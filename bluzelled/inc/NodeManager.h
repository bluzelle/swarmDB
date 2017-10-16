#ifndef KEPLER_NODEMANAGER_H
#define KEPLER_NODEMANAGER_H
#include "Node.h"

class NodeManager {
    long    max_nodes_;
    Nodes   nodes_;
public:
    long& max_nodes()   {return this->max_nodes_;}
    Nodes&  nodes()     {return this->nodes_;}

    void make_introductions() {}

    void kill_all_nodes() {}

    void join_all_threads() {}

    bool all_nodes_alive()
    {
        for(auto node : nodes_)
            {
            if(node->state() != Task::alive)
                {
                return false;
                }
            }
        return !nodes_.empty();
    }

    void run()
    {

    }

private:
    Node* create_node()
    {
        return nullptr;
    }

    Nodes create_nodes(const long number_of_nodes)
    {
        return Nodes();
    }

    void add_nodes()
    {}

    void initialization()
    {
        nodes_ = create_nodes(5);
        wait_for_all_nodes_to_start();
    }

    void main()
    {

    }

    void clean_up()
    {}

    void wait_for_all_nodes_to_start()
    {
        while (!all_nodes_alive())
            {
            boost::this_thread::sleep(boost::posix_time::milliseconds(100));
            }
    }

    void reaper()
    {
        join_dead_tasks();
        remove_dead_nodes();
    }

    void join_dead_tasks()
    {
        for(auto n : nodes_ )
            {
            if(n->state() == Task::dead)
                {
                n->join();
                }
            }
    }

    void remove_dead_nodes()
    {
        auto is_task_dead = [](auto n) {return (n->state() == Task::dead);};
        auto r = std::remove_if
                (
                        nodes_.begin(),
                        nodes_.end(),
                        is_task_dead
                );
        if(r != nodes_.end())
            {
            nodes_.erase(r,nodes_.end());
            }
    }




};

unsigned long number_of_nodes_to_create(unsigned long min_nodes, unsigned long max_nodes, unsigned current_number_of_nodes);

#endif //KEPLER_NODEMANAGER_H

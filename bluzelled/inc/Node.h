#ifndef KEPLER_NODE_H
#define KEPLER_NODE_H

#include "Task.h"
#include <boost/thread.hpp>

class Node
{
    Task*               _task;
    boost::thread*      _thread;

public:
    Node()
    {
        auto thread_function = []
                (
                        Task *task
                )
            {
            task->run();
            };

        _task   = new Task();
        _thread = new boost::thread
                (
                        thread_function,
                        _task
                );
    }

    boost::thread::id get_id()
    {
        return _task->get_id();
    }

    void kill()
    {
        _task->kill();
    }

    void join()
    {
        _thread->join();
    }

    Task::State state()
    {
        return _task->state();
    }

    bool is_joinable()
    {
        return _thread->joinable();
    }

    void ping(boost::thread::id other)
    {
        _task->ping(other);
    }

private:



};

typedef std::vector<Node *> Nodes;

#endif //KEPLER_NODE_H

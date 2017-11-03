#ifndef KEPLER_NODE_H
#define KEPLER_NODE_H

#include "Task.h"

#include <vector>
#include <boost/random.hpp>
#include <boost/random/random_device.hpp>
#include <boost/thread.hpp>

class Listener;

class Node {
    std::string name_;
    boost::shared_ptr<Task> task_;
    boost::shared_ptr<boost::thread> thread_;
    std::weak_ptr<Listener> listener_;

public:
    explicit Node(std::shared_ptr<Listener> l, uint32_t lifespan = 20, double death_probability = 0.05);
    boost::thread::id get_thread_id();
    void kill();
    void join();
    Task::State state();
    const std::string& name();
    bool is_joinable();
    void ping(boost::thread::id other);
    system_clock::time_point last_change();

private:
    void on_birth();
    void on_death();
    std::string generate_name();
};

typedef std::vector<Node *> Nodes;

#endif //KEPLER_NODE_H

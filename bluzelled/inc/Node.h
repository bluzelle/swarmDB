#ifndef KEPLER_NODE_H
#define KEPLER_NODE_H

#include <vector>

#include <boost/random.hpp>
#include <boost/random/random_device.hpp>
#include <boost/thread.hpp>

#include "Task.h"

class Node;

typedef std::vector<Node *> Nodes;

#include "web_sockets/Listener.h"


class Node {
    std::string name_;
    boost::shared_ptr<Task> task_;
    boost::shared_ptr<boost::thread> thread_;
    std::weak_ptr<Listener> listener_;
public:
    Node(std::shared_ptr<Listener> l, uint32_t lifespan = 20, double death_probablity = 0.05)
        : listener_(l)
    {
        auto thread_function = [this]
                (
                        Task *task
                )
            {
            task->run();
            on_death();
            };

        task_.reset(new Task(lifespan,death_probablity));
        thread_.reset(new boost::thread
                              (
                                      thread_function,
                                      task_.get()
                              ));

        name_ = generate_name(); //boost::lexical_cast<std::string>(get_thread_id());
        on_birth();
    }

    boost::thread::id get_thread_id() {
        return thread_->get_id();
    }

    void kill() {
        task_->kill();
    }

    void join() {
        thread_->join();
    }

    Task::State state() {
        return task_->state();
    }

    std::string name() {
        return name_;
    }

    bool is_joinable() {
        return thread_->joinable();
    }

    void ping(boost::thread::id other) {
        task_->ping(other);
    }

    system_clock::time_point last_change() {
        return task_->get_last_change();
    }

    void on_birth() {
        auto lp = listener_.lock();
        if (lp !=  nullptr) {
            for (auto s : lp->sessions_)
                {
                auto sp = s.lock();
                if (!sp)  // corresponding shared_ptr is destroyed (session closed).
                    {
                    lp->sessions_.erase(
                            std::find_if(
                                    lp->sessions_.begin(),
                                    lp->sessions_.end(),
                                    [&sp](auto n)
                                        { return sp == n.lock(); }));
                    }
                else
                    sp->send_update_nodes(name_);
                }
            }
    }

    void on_death() {
        auto lp = listener_.lock();
        if (lp !=  nullptr) {
            for (auto s : lp->sessions_)
                {
                auto sp = s.lock();
                if (!sp)  // corresponding shared_ptr is destroyed (session closed).
                    {
                    lp->sessions_.erase(
                            std::find_if(
                                    lp->sessions_.begin(),
                                    lp->sessions_.end(),
                                    [&sp](auto n)
                                        { return sp == n.lock(); }));
                    }
                else
                    sp->send_remove_nodes(name_);
                }
            }
    }

    std::string generate_name() {
        boost::random::random_device dev;
        boost::random::mt19937 rng(dev);

        boost::random::uniform_int_distribution<> two_two_five(0,255);

        int ip[4] = {0};
        for (auto& addr : ip) {
            addr = two_two_five(rng);
            }

        auto f = boost::format("%d.%d.%d.%d") % ip[0] % ip[1] % ip[2] % ip[3];
        return boost::str(f);
    }

private:

};

#endif //KEPLER_NODE_H

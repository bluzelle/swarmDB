#include "Node.h"
#include "NodeUtilities.h"
#include "web_sockets/Listener.h"

void thread_function(Task *t, boost::exception_ptr &error);

Node::Node(std::shared_ptr<Listener> l, uint32_t lifespan, double death_probability)
        : listener_(l) {
    auto thread_function = [this]
            (
                    Task *task
            )
        {
        task->run();
        on_death();
        };

    task_.reset(new Task(lifespan, death_probability));
    thread_.reset(new boost::thread
                          (
                                  thread_function,
                                  task_.get()
                          ));

    name_ = generate_name();
    on_birth();
}

boost::thread::id Node::get_thread_id() {
    return thread_->get_id();
}

void Node::kill() {
    task_->kill();
}

void Node::join() {
    thread_->join();
}

Task::State Node::state() {
    return task_->state();
}

const std::string &Node::name() {
    return name_;
}

bool Node::is_joinable() {
    return thread_->joinable();
}

void Node::ping(boost::thread::id other) {
    task_->ping(other);
}

system_clock::time_point Node::last_change() {
    return task_->get_last_change();
}

void Node::on_birth() {
    auto lp = listener_.lock();
    if (lp != nullptr)
        {
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

void Node::on_death() {
    auto lp = listener_.lock();
    if (lp != nullptr)
        {
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

std::string Node::generate_name() {
    return generate_ip_address();
}

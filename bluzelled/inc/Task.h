#ifndef KEPLER_TASK_H
#define KEPLER_TASK_H

#include "CSet.h"
#include <sstream>
#include <boost/thread.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/chrono.hpp>
#include <iostream>

using namespace boost::chrono;

#define RAND() ((double)rand() / RAND_MAX )

class Task
{
public:
    enum State {
        initializing, alive, dying, dead
    };

    std::string state_string(State s)
    {
        return (const char*[]){"initializing", "alive", "dying", "dead"}[s];
    }

private:
    double                      death_probability_;
    thread_clock::duration      lifespan_;
    thread_clock::time_point    birth_;
    State                       state_;
    CSet<boost::thread::id>     *peers_;
    system_clock::time_point    state_changed_;

public:
    Task(uint32_t lifespan = 20, double death_prob = 0.05)
    {
        lifespan_ = boost::chrono::seconds(lifespan);
        death_probability_ = death_prob;
    }

    ~Task()
    {
        if (peers_ != nullptr)
            {
            peers_->clear();
            delete peers_;
            peers_ = nullptr;
            }
    }

    void run()
    {
        setup();
        life();
        death();
    }

    void operator()()
    {
        run();
    }

    void ping(const boost::thread::id &src_id)
    {
        (void) src_id;

//        std::stringstream s;
//        s << "[" << get_id() << "]" << src_id << " pinged me!\n";
//        boost::this_thread::yield();
//        print_message(s.str().c_str());
//
//        if(_friends && _friends->end() != _friends->find(src_id))
//            {
//            s.str("");
//            s << "we're already friends!\n";
//            }
//        else
//            {
//            Task* t = find_task(src_id);
//            if (nullptr != t)
//                {
//                s.str("");
//                s << "I've made a new friend! Pinging back!\n";
//                _friends->insert(src_id);
//                t->ping(get_id());
//                }
//            else
//                {
//                s << "Friend not found!\n";
//                }
//            }
//
//        print_message(s.str().c_str());
    }

    State state()
    {
        return state_;
    }

    void kill()
    {
        state_ = dying;
    }

    system_clock::time_point get_last_change()
    {
        return state_changed_;
    }

    unsigned long peer_count()
    { return peers_ != nullptr ? peers_->size() : 0; }

private:
    void setup()
    {
        birth_ = thread_clock::now();
        state_changed_ = system_clock::now();
        srand(time(0));
        state_ = initializing;
        peers_ = new CSet<boost::thread::id>();
    }

    void life()
    {
        state_changed_ = system_clock::now();
        state_ = alive;
        while (state() == alive)
            {
            state_ = (check_for_natural_death() ? Task::dying : state_);



            }
    }

    void death()
    {
        state_ = dead;
        state_changed_ = system_clock::now();
    }


    ///////////////////////////////////////
    bool check_for_natural_death()
    {
        thread_clock::duration age = thread_clock::now().time_since_epoch() - birth_.time_since_epoch();
        bool lifespan_end = (age > lifespan_);
        bool death_p = (RAND() < death_probability_);
        return lifespan_end && death_p;
    }


    //
    void interact()
    {
//        _friends.iterate(
//                [
//                        this
//                ](
//                        boost::thread::id i
//                )
//                    {
//
//
//                    }
//        );
    }
};


#endif //KEPLER_TASK_H

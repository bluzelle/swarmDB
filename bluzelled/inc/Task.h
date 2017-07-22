#ifndef KEPLER_TASK_H
#define KEPLER_TASK_H

#include "CSet.h"
#include <sstream>
#include <boost/thread.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/chrono.hpp>


using namespace boost::chrono;

#define RAND() ((double)rand() / RAND_MAX )


class Task {
public:
    enum  State { initializing, alive, dying, dead};

private:
    boost::thread::id           _id;
    thread_clock::time_point    _birth;
    State                       _state;
    CSet<boost::thread::id>*    _friends;

public:
    ~Task()
    {
        if(_friends != nullptr)
            {
            _friends->clear();
            delete _friends;
            _friends = nullptr;
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

    void ping(const boost::thread::id& src_id)
    {
        (void)src_id;

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

    boost::thread::id get_id()
    {
        return _id;
    }

    State state()
    {
        return _state;
    }

    void kill()
    {
        _state=dying;
    }

    unsigned long friendCount() { return _friends != nullptr ? _friends->size() : 0 ;}

private:
    void setup()
    {
        _birth = thread_clock::now();
        srand(time(0));

        _state = initializing;
        //print_message("startup begin\n");
        //boost::this_thread::yield();
        _id = boost::this_thread::get_id();
        _friends = new CSet<boost::thread::id>();
        //print_message("startup end\n");
    }

    void life()
    {
        _state = alive;
        //print_message("life\n");
        while( state() == alive )
            {
            thread_clock::duration age = thread_clock::now().time_since_epoch() - _birth.time_since_epoch();
            if(age > boost::chrono::seconds(20))
                {
                if(RAND() < 0.25)
                    {
                    _state = Task::dying;
                    }
                }
            boost::this_thread::yield();
            }
    }

    void death()
    {
//        std::stringstream ss;
//        ss << "I'm dying with " << friendCount() << " friends!\n";
//        print_message(ss.str());
        _state = dead;
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

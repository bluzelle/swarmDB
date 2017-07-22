#ifndef KEPLER_BZSYNCMAPQUEUE_H
#define KEPLER_BZSYNCMAPQUEUE_H

#include <queue>
#include <thread>
#include <boost/thread.hpp>
#include <condition_variable>

template<typename Data>
class CQueue
{
private:
    std::queue<Data> _queue;
    mutable boost::mutex _mutex;
public:
    void push(const Data& data)
    {
        boost::mutex::scoped_lock lock(_mutex);
        _queue.push(data);
    }

    bool empty() const
    {
        boost::mutex::scoped_lock lock(_mutex);
        return _queue.empty();
    }

    Data& front()
    {
        boost::mutex::scoped_lock lock(_mutex);
        return _queue.front();
    }

    Data const& front() const
    {
        boost::mutex::scoped_lock lock(_mutex);
        return _queue.front();
    }

    void pop()
    {
        boost::mutex::scoped_lock lock(_mutex);
        _queue.pop();
    }
};

#endif //KEPLER_BZSYNCMAPQUEUE_H

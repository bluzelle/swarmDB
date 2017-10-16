#ifndef KEPLER_CSET_H
#define KEPLER_CSET_H
#define BOOST_USE_VALGRIND
#include <set>
#include <boost/thread.hpp>

template <typename T>
class CSet {
private:
    boost::mutex _mutex;
    std::set<T> _set;

public:
    typedef std::set<T>  csetType;

    bool insert(const T& object)
    {
        return _set.insert(object).second;
    }

    bool cInsert(const T& object)
    {
        boost::mutex::scoped_lock lock(_mutex);
        return insert(object);
    }

    unsigned long size()
    {
        boost::mutex::scoped_lock lock(_mutex);
        return _set.size();
    }

    template<typename Functor>
    void iterate(Functor f)
    {
        boost::mutex::scoped_lock lock(_mutex);
        for(auto i : _set)
            {
            f(i);
            }
    }

    void remove(typename std::set<T>::iterator it)
    {
        boost::mutex::scoped_lock lock(_mutex);
        if(end() != _set.find(*it))
            {
            _set.erase(it);
            }
    }

    auto find(const T &object)
    {
        boost::mutex::scoped_lock lock(_mutex);
        return _set.find(object);
    }

    auto end()
    {
        return _set.end();
    }

    void clear()
    {
        boost::mutex::scoped_lock lock(_mutex);
        _set.clear();
    }

    bool empty()
    {
        boost::mutex::scoped_lock lock(_mutex);
        return _set.empty();
    }
};

#endif //KEPLER_CSET_H

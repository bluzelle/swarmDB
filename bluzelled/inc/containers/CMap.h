#ifndef KEPLER_CMAP_H
#define KEPLER_CMAP_H

#include <map>
#include <boost/thread.hpp>


template <typename Key, typename T>
class CMap
{
private:
    boost::mutex _mutex;
    std::map<Key, T> _map;

public:
    bool insert(const Key& key, const T& v)
    {
        if( _map.end() == _map.find(key))
            {
            return _map.insert(std::make_pair(key,v)).second;
            }
        return false;
    }

    bool cInsert(const Key& key, const T& v) {
        boost::mutex::scoped_lock lock(_mutex);
        return insert(key, v);
    }

    T get(Key& key)
    {
        return (*_map.find(key)).second;
    }

    unsigned long size()
    {
        return _map.size();
    }

};

#endif //KEPLER_CMAP_H

#ifndef KEPLER_BZSYNCMAPWRAPPER_HPP
#define KEPLER_BZSYNCMAPWRAPPER_HPP
#include <iostream>
#include <map>
#include <mutex>

using namespace std;

template <class T, class Y>
class BZSyncMapWrapper
{
public:
    //typedef map<int, std::string> BZSyncMapType;
    typedef map<T, Y> BZSyncMapType;

    bool safeInsert(T &key, Y &object)
    {
        bool retval = false;
        std::unique_lock<std::mutex> mlock(m_mutex, defer_lock);
        mlock.lock();
        retval = unsafeInsert(key,object);
        mlock.unlock();
        return retval;
    }

    bool unsafeInsert(T &key, Y &object)
    {
        return m_unsafeMap.insert(
                BZSyncMapType::value_type( key, object)
        ).second;
    }

    template <class Functor>
    void safeIterate(Functor function)
    {
        std::unique_lock<std::mutex> mutexLock(m_mutex, std::defer_lock);
        mutexLock.lock();
        for (auto it : m_unsafeMap)
            {
            cout << "[" << it.first << "][" << it.second << "]\n";
            function(it.first, it.second);
            }
        mutexLock.unlock();
    }

    template <class Functor>
    void safeUse(Functor function)
    {
        function(m_unsafeMap);
    }

    template <class Functor>
    void safeUseMember(const T &key, Functor function)
    {
        function(key);
    }

    unsigned long GetSize() {return m_unsafeMap.size();};

private:
    BZSyncMapType  m_unsafeMap;
    mutex           m_mutex;
};

#endif //KEPLER_BZSYNCMAPWRAPPER_HPP

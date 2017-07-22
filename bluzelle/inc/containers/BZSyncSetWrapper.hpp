#ifndef KEPLER_BZSYNCSETWRAPPER_H
#define KEPLER_BZSYNCSETWRAPPER_H

#include <iostream>
#include <set>
#include <mutex>

using namespace std;

template <class T>
class BZSyncSetWrapper
{
public:
    typedef set<T>  BZSyncSetType;

    bool safeInsert(T& object)
    {
        bool p;
        std::unique_lock<std::mutex> m(m_mutex, std::defer_lock);
        m.lock();
        p = unsafeInsert(object);
        m.unlock();
        return p;
    }

    bool unsafeInsert(T& object)
    {
        return m_unsafeSet.insert(object).second;
    }

    unsigned long Size()
    {
        return m_unsafeSet.size();
    }

    template<typename Functor>
    void safeIterate(Functor function)
    {
        std::unique_lock<std::mutex> m(m_mutex, std::defer_lock);
        m.lock();
        for(auto i : m_unsafeSet)
            {
            function(i);
            }
        m.unlock();

    }

    template <typename Functor>
    void safeUse(Functor function)
    {
        std::unique_lock<std::mutex> m(m_mutex, std::defer_lock);
        m.lock();
        function(m_unsafeSet);
        m.unlock();
    }

private:
    BZSyncSetType   m_unsafeSet;
    std::mutex      m_mutex;
};
#endif //KEPLER_BZSYNCSETWRAPPER_H

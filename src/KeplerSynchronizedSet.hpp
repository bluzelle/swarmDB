#ifndef KEPLERSYNCHRONIZEDSET_H
#define KEPLERSYNCHRONIZEDSET_H



#include <iostream>
#include <thread>
#include <set>



template <class T> 
class KeplerSynchronizedSet
    {
    public:

        bool safe_insert(T &object);

    private:

        typedef std::set<T> KeplerSynchronizedSetType;

        KeplerSynchronizedSetType m_unsafeSet;

        std::mutex m_mutex;               
    };



template <class T>
inline bool KeplerSynchronizedSet<T>::safe_insert(T &object)
    {
    typename KeplerSynchronizedSetType::iterator iteratorReturnValueFromInsertion;

    std::unique_lock<std::mutex> mutexLock(m_mutex,
                                           std::defer_lock);

    mutexLock.lock();

    iteratorReturnValueFromInsertion = m_unsafeSet.insert(object);

    mutexLock.unlock();

    return iteratorReturnValueFromInsertion->second;
    }

    

#endif // KEPLERSYNCHRONIZEDSET_H
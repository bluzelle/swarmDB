#ifndef KEPLERSYNCHRONIZEDSETWRAPPER_H
#define KEPLERSYNCHRONIZEDSETWRAPPER_H



#include <iostream>
#include <set>
#include <mutex>



template <class T> 
class KeplerSynchronizedSetWrapper
    {
    public:

        typedef std::set<T> KeplerSynchronizedSetType;

        bool safe_insert(T &object);
        bool unsafe_insert(T &object);

        // Example usage:
        //
	    // KeplerApplication::s_threads.safe_iterate([] (const std::shared_ptr<std::thread> &ptr_newThread) 
	    //     {
	    //     std::cout << ptr_newThread->get_id() << std::endl;
	    //     });

        template<typename Functor>
        void safe_iterate(Functor function)
	        {
		    std::unique_lock<std::mutex> mutexLock(m_mutex,
		                                           std::defer_lock);

		    mutexLock.lock();

			for (typename KeplerSynchronizedSetType::iterator iteratorSet = m_unsafeSet.begin(); 
				 iteratorSet != m_unsafeSet.end(); 
				 ++iteratorSet) 
			    {
			    const T &t = *iteratorSet;

			    function(t);
			    }

		    mutexLock.unlock();
	        }

        template<typename Functor>
        void safe_use(Functor function)
	        {
		    std::unique_lock<std::mutex> mutexLock(m_mutex,
		                                           std::defer_lock);

		    mutexLock.lock();

		    function(m_unsafeSet);

		    mutexLock.unlock();
	        }

    private:

        KeplerSynchronizedSetType m_unsafeSet;

        std::mutex m_mutex;               
    };



template <class T>
inline bool KeplerSynchronizedSetWrapper<T>::safe_insert(T &object)
    {
    std::pair<typename KeplerSynchronizedSetType::iterator, bool> pairReturnValueFromInsertion;

    std::unique_lock<std::mutex> mutexLock(m_mutex,
                                           std::defer_lock);

    mutexLock.lock();

    pairReturnValueFromInsertion = m_unsafeSet.insert(object);

    mutexLock.unlock();

    return pairReturnValueFromInsertion.second;
    }

// ONLY use this if you are already in a critical section when you need to an insertion

template <class T>
inline bool KeplerSynchronizedSetWrapper<T>::unsafe_insert(T &object)
    {
    std::pair<typename KeplerSynchronizedSetType::iterator, bool> pairReturnValueFromInsertion;

    pairReturnValueFromInsertion = m_unsafeSet.insert(object);

    return pairReturnValueFromInsertion.second;
    }

#endif // KEPLERSYNCHRONIZEDSET_H
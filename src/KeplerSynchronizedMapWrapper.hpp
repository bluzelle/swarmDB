#ifndef KEPLERSYNCHRONIZEDMAPWRAPPER_H
#define KEPLERSYNCHRONIZEDMAPWRAPPER_H



#include <iostream>
#include <map>
#include <mutex>



template <class T, class Y> 
class KeplerSynchronizedMapWrapper
    {
    public:

        typedef std::map<T, Y> KeplerSynchronizedMapType;

        bool safe_insert(T &key, Y &object)
            {
            std::pair<typename KeplerSynchronizedMapType::iterator, bool> pairReturnValueFromInsertion;

            std::unique_lock<std::mutex> mutexLock(m_mutex,
                                                   std::defer_lock);

            mutexLock.lock();

            pairReturnValueFromInsertion = m_unsafeMap.insert(KeplerSynchronizedMapType::value_type(key, object));

            mutexLock.unlock();

            return pairReturnValueFromInsertion.second;
            }

        // ONLY use this if you are already in a critical section when you need to an insertion

        bool unsafe_insert(T &key, Y &object)
            {
            std::pair<typename KeplerSynchronizedMapType::iterator, bool> pairReturnValueFromInsertion;

            pairReturnValueFromInsertion = m_unsafeMap.insert(KeplerSynchronizedMapType::value_type(key, object));

            return pairReturnValueFromInsertion.second;
            }

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

			for (typename KeplerSynchronizedMapType::iterator iteratorMap = m_unsafeMap.begin(); 
				 iteratorMap != m_unsafeMap.end(); 
				 ++iteratorMap) 
			    {
			    const T &t = iteratorMap->first;
                const Y &y = iteratorMap->second;

			    function(t, y);
			    }

		    mutexLock.unlock();
	        }

        // KeplerApplication::s_threads.safe_use([&myThreadId] (KeplerSynchronizedMapWrapper<std::thread::id, std::shared_ptr<std::thread>>::KeplerSynchronizedMapType &mapThreads) 
        //     {
        //     });

        template<typename Functor>
        void safe_use(Functor function)
	        {
		    std::unique_lock<std::mutex> mutexLock(m_mutex,
		                                           std::defer_lock);

		    mutexLock.lock();

		    function(m_unsafeMap);

		    mutexLock.unlock();
	        }

    private:

        KeplerSynchronizedMapType m_unsafeMap;

        std::mutex m_mutex;               
    };

#endif // KEPLERSYNCHRONIZEDMAPWRAPPER_H
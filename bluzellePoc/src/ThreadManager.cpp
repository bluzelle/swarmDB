#include "ThreadManager.h"
#include "KeplerApplication.hpp"
#include "KeplerFrame.hpp"
#include "ThreadUtilities.h"
//#include "Utility.h"

#include <boost/range/irange.hpp>

#include <sstream>

#define MAX_THREADS 25

void crashMessage(const std::thread::id &threadId) {
    std::stringstream stringStreamOutput1;
    stringStreamOutput1
            << "Node with id: "
            << threadId
            << " has 'decided' to 'crash'... joining it..."
            << std::endl;
    KeplerFrame::s_ptr_global->addTextToTextCtrlApplicationWideLogQueue(stringStreamOutput1.str());
    lockedCout(stringStreamOutput1.str());
}

void joinMessage(const std::thread::id &threadId) {
    std::stringstream stringStreamOutput2;
    stringStreamOutput2
            << "Node with id: "
            << threadId
            << " joining DONE"
            << std::endl;
    KeplerFrame::s_ptr_global->addTextToTextCtrlApplicationWideLogQueue(stringStreamOutput2.str());
    lockedCout(stringStreamOutput2.str());
}

void nodeCountMessage(const int intThreadCount) {
    std::stringstream stringStreamOutput1;
    stringStreamOutput1 << "Number of nodes in map: "
                        << intThreadCount << std::endl;
    KeplerFrame::s_ptr_global->addTextToTextCtrlApplicationWideLogQueue(stringStreamOutput1.str());
    lockedCout(stringStreamOutput1.str());
}

void nodeRecountMessage(const int intThreadCount) {
    std::stringstream stringStreamOutput2;
    stringStreamOutput2
            << "Number of nodes in map is NOW: "
            << intThreadCount << std::endl;
    KeplerFrame::s_ptr_global->addTextToTextCtrlApplicationWideLogQueue(stringStreamOutput2.str());
    lockedCout(stringStreamOutput2.str());
}

// thread related
void ThreadManager::createNewThreadsIfNeeded() {
    if (KeplerApplication::s_bool_endAllThreads == false)
        {
        std::vector<std::thread::id> vectorNewlyCreatedThreadIds;

        // Access the threads object inside a critical section

        KeplerApplication::s_threads.safe_use
                ([
                         &vectorNewlyCreatedThreadIds
                 ](
                        KeplerSynchronizedMapWrapper<std::thread::id, ThreadData>::KeplerSynchronizedMapType &mapThreads
                )
                     {
                     if (mapThreads.size() < MAX_THREADS)
                         {
                         nodeCountMessage(mapThreads.size());
                         // We don't actually create all the deficit threads,
                         // necessarily. We choose the number to create
                         // randomly, from 1 to the actual total number needed
                         const int uintMaximumNewThreadsToCreate = (MAX_THREADS - mapThreads.size());
                         const int uintNumberOfNewThreadsToActuallyCreate = (
                                 (getThreadFriendlyLargeRandomNumber() % uintMaximumNewThreadsToCreate) + 1
                         );

                         for (const unsigned int i : boost::irange(0, uintNumberOfNewThreadsToActuallyCreate))
                             {
                             std::shared_ptr<std::thread> ptr_newThread(new std::thread(threadFunction, i));

                             // Is this safe? Do we now have two ref-counted objects pointing to the thread but each with a count of 1?

                             mapThreads.insert(
                                     KeplerSynchronizedMapWrapper<std::thread::id, std::shared_ptr<std::thread>>::KeplerSynchronizedMapType::value_type(
                                             ptr_newThread->get_id(),
                                             ptr_newThread));

                             vectorNewlyCreatedThreadIds.push_back(
                                     ptr_newThread->get_id());
                             }
                         nodeRecountMessage(mapThreads.size());
                         }
                     });

        // TODO RHN need to make this work
        for (const auto &currentNewlyCreatedThreadId : vectorNewlyCreatedThreadIds)
            {

           //onNewlyCreatedThread(currentNewlyCreatedThreadId);
            }

        }


}

void ThreadManager::killAndJoinThreadsIfNeeded() {

    std::vector<std::thread::id> vectorNewlyKilledThreadIds;
    // Access the threads object inside a critical section
    KeplerApplication::s_threads.safe_use(
            [&vectorNewlyKilledThreadIds](
                    KeplerSynchronizedMapWrapper<std::thread::id, ThreadData>::KeplerSynchronizedMapType &mapThreads)
                {
                KeplerApplication::s_threadIdsToKill.safe_iterate(
                        [
                                &mapThreads,
                                &vectorNewlyKilledThreadIds
                        ](
                                const std::thread::id &threadId
                        )
                            {
                            crashMessage(threadId);
                            if (mapThreads.at(
                                    threadId).m_ptr_thread->joinable())
                                {
                                mapThreads.at(
                                        threadId).m_ptr_thread->join();
                                }
                            joinMessage(threadId);
                            mapThreads.erase(threadId);
                            vectorNewlyKilledThreadIds.push_back(threadId);
                            });
                });

    // WATCHOUT -- there could be a race condition newly deleted threads get into the s_threadIdsToKill map before the code below fires, which means those
    // newly deleted threads don't get processed as above. Maybe. Not sure. The deleting of only items in the vector probably ensures this cannot happen.

    KeplerApplication::s_threadIdsToKill.safe_use([&vectorNewlyKilledThreadIds](
            KeplerSynchronizedSetWrapper<std::thread::id>::KeplerSynchronizedSetType &setThreadsToKill)
                                                      {
                                                      for (const auto &currentNewlyKilledThreadId : vectorNewlyKilledThreadIds)
                                                          {
                                                          setThreadsToKill.erase(currentNewlyKilledThreadId);

//            onNewlyCreatedThread(currentNewlyCreatedThreadId);
                                                          }
                                                      });

//    for (const auto &currentNewlyKilledThreadId : vectorNewlyKilledThreadIds)
//        {
///// TODO        onNewlyKilledThread(currentNewlyKilledThreadId);
//        }
}




#include "BZTask.h"
#include "ThreadManager.h"
#include "BZApplicationLogListView.h"
#include <thread>
#include <sstream>
#include <iostream>
#include <boost/random/linear_congruential.hpp>

#define THREAD_RANDOM_LOG_ARBITRARY_MESSAGE_PROBABILITY_PERCENTAGE 1
#define THREAD_RANDOM_DEATH_PROBABILITY_PERCENTAGE 0.1
#define THREAD_SLEEP_TIME_MILLISECONDS 40


void introduction(const uint32_t i);
void lifeCycle(const uint32_t i);
void lockedCout(const std::string &str);
void threadLifeCycleLoop(const std::thread::id &myThreadId);
void death( std::thread::id& threadId);

void task(const uint32_t i) // was threadFunction
{
    introduction(i);
    lifeCycle(i);
}

void introduction(const uint32_t i) // was threadIntroduction
{
//    std::stringstream outStream;
//    outStream << "(" << i << ")"
//              << "A NEW node just came up with id:"
//              << std::this_thread::get_id()
//              << "\n";
//    lockedCout(outStream.str());
    // TODO update kepler frame: KeplerFrame::s_ptr_global->addTextToTextCtrlApplicationWideLogQueue(stringStreamOutput.str());
}

void lifeCycle(const uint32_t i)
{
    bool shouldDie = false;
    std::thread::id threadId = std::this_thread::get_id();
    do
        {
        uint32_t sleepTime = rand() % THREAD_SLEEP_TIME_MILLISECONDS + 10; // TODO use boost random

//        std::stringstream outStream;
//        outStream << "(" << i << ")"
//                  << "Node " << threadId
//                  << "awakens after "
//                  << sleepTime << "ms and goes back to sleep.\n";
//        BZRootFrame::pushMessage("log",outStream.str().c_str());
        threadLifeCycleLoop(threadId);
        shouldDie = (((double)rand() /(double)RAND_MAX) * 100.0 <= THREAD_RANDOM_DEATH_PROBABILITY_PERCENTAGE );
        } while
            (!
            (ThreadManager::_endAllThreads || shouldDie)
            );
    death(threadId);
}

void threadLifeCycleLoop(const std::thread::id &threadId)
{
    double randomNumber = 100.0 * ((double)rand()/(double)RAND_MAX); // TODO use Boost random.
    const bool boolThreadRandomlyLogsArbitraryMessage = (randomNumber <= THREAD_RANDOM_LOG_ARBITRARY_MESSAGE_PROBABILITY_PERCENTAGE);

    if (boolThreadRandomlyLogsArbitraryMessage)
        {
//        std::stringstream str;
//        str
//            << "Thread with id: "
//            << threadId
//            << " now has "
//            << "<threadData.m_vectorLogMessages.size()>"
//            << " messages in it"
//            << std::endl;
//
//        BZRootFrame::pushMessage("log",str.str().c_str());
        }


}

void death( std::thread::id& threadId) {
    std::stringstream stringStreamOutput;
    stringStreamOutput << "Node is ending its lifecycle with id: "
                       <<  threadId
                       << std::endl
                       << std::flush;
    std::string strOutput = stringStreamOutput.str();


    std::unique_lock<std::mutex> m(BZRootFrame::s_rootMutex, std::defer_lock);
    m.lock();
    BZRootFrame::pushMessage("log", strOutput.c_str());
    BZRootFrame::pushMessage("remove", strOutput.c_str());
    BZRootFrame::GetInstance()->RemoveNodeListItem(threadId);
    m.unlock();
}




void lockedCout(const std::string &str) {
    //std::unique_lock<std::mutex> lockStdOut = KeplerApplication::getStdOutLock();
    std::cout << str;
    //lockStdOut.unlock();
}
#include "ThreadUtilities.h"

//#include <string>
#include <sstream>
#include <sys/time.h>
#include <random>


#include "KeplerFrame.hpp"
#include "KeplerApplication.hpp"
#include "Utility.h"

#define THREAD_RANDOM_LOG_ARBITRARY_MESSAGE_PROBABILITY_PERCENTAGE 1
#define THREAD_RANDOM_DEATH_PROBABILITY_PERCENTAGE 0.01
#define THREAD_SLEEP_TIME_MILLISECONDS 40

int getThreadFriendlyLargeRandomNumber() {
    typedef unsigned long long u64;
    u64 u64useconds;
    struct timeval tv = {};
    gettimeofday(&tv, NULL);
    u64useconds = (1000000 * tv.tv_sec) + tv.tv_usec;
    std::srand(u64useconds); // use current time as seed for random generator
    int intRandomVariable = std::rand();
    return intRandomVariable;
}

void lockedCout(const std::string &str) {
    std::unique_lock<std::mutex> lockStdOut = KeplerApplication::getStdOutLock();
    std::cout << str;
    lockStdOut.unlock();
}

void threadIntroduction(const unsigned int i) {
    std::stringstream stringStreamOutput;
    stringStreamOutput << "(" << i << ")"
                       << "A NEW node just came up with id: "
                       << std::this_thread::get_id() << "\n";
    KeplerFrame::s_ptr_global->addTextToTextCtrlApplicationWideLogQueue(stringStreamOutput.str());
    lockedCout(stringStreamOutput.str());
    // TODO RHN    KeplerFrame::s_ptr_global->addThreadToListViewNodes(std::this_thread::get_id());
}

void logArbitraryThreadLogMessage(
        const std::thread::id &myThreadId,
        const std::string &strLogMessage) {
    KeplerApplication::s_threads.safe_use_member(myThreadId,
                                                 [&strLogMessage,
                                                         &myThreadId](ThreadData &threadData)
                                                     {
                                                     threadData.m_vectorLogMessages.push_back(strLogMessage);

                                                     std::unique_lock<std::mutex> lockStdOut = KeplerApplication::getStdOutLock();

                                                     std::cout << "Thread with id: " << myThreadId << " now has "
                                                               << threadData.m_vectorLogMessages.size()
                                                               << " messages in it" << std::endl;
                                                     });
}

float randomValue() {
    return (getThreadFriendlyLargeRandomNumber() % 1000000) * 1.0f / 10000.0f;
}

void threadLifeCycleLoop(const std::thread::id &myThreadId) {
    const bool boolThreadRandomlyLogsArbitraryMessage = (randomValue() <=
                                                         THREAD_RANDOM_LOG_ARBITRARY_MESSAGE_PROBABILITY_PERCENTAGE);

    if (boolThreadRandomlyLogsArbitraryMessage)
        {
        logArbitraryThreadLogMessage(myThreadId,
                                     getStringFromThreadId(myThreadId));
        }
}

void threadLifeCycle(const unsigned int i) {
    bool boolThreadRandomlyShouldDie = false;
    do
        {
        unsigned int uintActualMillisecondsToSleep = KeplerApplication::sleepRandomMilliseconds(
                THREAD_SLEEP_TIME_MILLISECONDS);
        std::thread::id myThreadId = std::this_thread::get_id();
        std::stringstream stringStreamOutput;
        stringStreamOutput << "Node awakens after "
                           << uintActualMillisecondsToSleep
                           << "ms and then goes back to sleep with id: "
                           << myThreadId
                           << std::endl
                           << std::flush;
        std::string strOutput = stringStreamOutput.str();

//TODO RHN    KeplerFrame::s_ptr_global->addTextToTextCtrlApplicationWideLogQueue(strOutput);
        boolThreadRandomlyShouldDie = (randomValue() <= THREAD_RANDOM_DEATH_PROBABILITY_PERCENTAGE);

        threadLifeCycleLoop(myThreadId);
        }  while ((!KeplerApplication::s_bool_endAllThreads) &&
           (!boolThreadRandomlyShouldDie));

    std::thread::id myThreadId = std::this_thread::get_id();

    std::stringstream stringStreamOutput;

    stringStreamOutput
            << "(" << i << ")" // have to use i
            << "Node is ending its lifecycle with id: "
            << myThreadId
            << std::endl
            << std::flush;

    std::string strOutput = stringStreamOutput.str();

    KeplerFrame::s_ptr_global->addTextToTextCtrlApplicationWideLogQueue(strOutput);

    // We only flag the thread to be killed manually if it naturally decided to die AND the system is not already
    // set to end all threads. If the latter is true, this thread will be killed anyways. We certainly don't
    // want it being killed twice -- that will cause a crash.

    if ((boolThreadRandomlyShouldDie) &&
        (!KeplerApplication::s_bool_endAllThreads))
        {
        KeplerApplication::s_threadIdsToKill.safe_insert(myThreadId);
        }

    lockedCout(strOutput);
}

void threadFunction(const unsigned int i) {
    threadIntroduction(i);
    threadLifeCycle(i);
}

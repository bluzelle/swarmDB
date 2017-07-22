#include "ThreadManager.h"
#include "BZRootFrame.h"
#include "BZTask.h"

#include <iostream>
#include <boost/format.hpp>
#include <vector>
#include <thread>
#include <boost/random/linear_congruential.hpp>
#include <boost/range/irange.hpp>

#define MAX_THREADS 25

using namespace std;
unique_ptr<ThreadManager> ThreadManager::m_instance;
once_flag ThreadManager::m_onceFlag;
bool ThreadManager::_endAllThreads = false;

BZSyncMapWrapper<std::thread::id, ThreadData> ThreadManager::s_threads;
BZSyncSetWrapper<std::thread::id> ThreadManager::s_threadIdsToKill;


ThreadManager& ThreadManager::GetInstance()
{
    std::call_once(m_onceFlag,[]{
        m_instance.reset(new ThreadManager);
        });
    return *m_instance.get();
}

std::vector<std::string> ThreadManager::GetData()
{
    std::vector<std::string> out = {"hello", "there", "bluzelle!"};
    return out;
}

void ThreadManager::createNewThreadsIfNeeded() // line 874 main.cpp
{
    BZRootFrame::pushMessage("log","ThreadManager::createNewThreadsIfNeeded()");
    if(!ThreadManager::_endAllThreads)
        {
        vector<thread::id> newlyCreatedThreadIds;
        ThreadManager::s_threads.safeUse( // 882
                [
                        &newlyCreatedThreadIds
                ](
                        BZSyncMapWrapper<std::thread::id, ThreadData>::BZSyncMapType &mapThreads
                )
                    {
                    const int threadCount = mapThreads.size();
                    if(threadCount < MAX_THREADS)
                        {
                        stringstream stringStreamOutput1;
                        stringStreamOutput1 << "Number of nodes in map: " << threadCount << std::endl;
                        const string strOutput1 = stringStreamOutput1.str();
                        BZRootFrame::pushMessage("log", strOutput1.c_str());

                    //    KeplerFrame::s_ptr_global->addTextToTextCtrlApplicationWideLogQueue(strOutput1);
                        double rndNum = (double)rand()/(double)RAND_MAX;

                        int numThreadsToCreate = (MAX_THREADS - threadCount) * rndNum;

                        for(const uint8_t i : boost::irange(0,numThreadsToCreate))
                            {// line 910
                            std::shared_ptr<std::thread> newThread(new std::thread(task, i));
                            mapThreads.insert(BZSyncMapWrapper<thread::id, std::shared_ptr<thread>>::BZSyncMapType::value_type(newThread->get_id(), newThread));
                            newlyCreatedThreadIds.emplace_back(newThread->get_id());
                            BZRootFrame::GetInstance()->AddNodeListItem(newThread->get_id());
                            }

                        std::stringstream stringStreamOutput2;
                        stringStreamOutput2 << "Number of nodes in map is NOW: " << mapThreads.size() << std::endl;
                        const std::string strOutput2 = stringStreamOutput2.str();
                        BZRootFrame::pushMessage("log", strOutput2.c_str());

                        }

                    }

        ); // 931






        }
}

void ThreadManager::killAndJoinThreadsIfNeeded()//line 809
{
    //BZRootFrame::pushMessage("log","ThreadManager::killAndJoinThreadsIfNeeded()");
    std::vector<std::thread::id> newlyKilledThreadIDs;
    ThreadManager::s_threads.safeUse(
            [
                    &newlyKilledThreadIDs
            ](
                    BZSyncMapWrapper<std::thread::id, ThreadData>::BZSyncMapType& mapThreads
            )
                {
                ThreadManager::s_threadIdsToKill.safeIterate(
                        [
                                &mapThreads,
                                &newlyKilledThreadIDs

                        ](
                                const std::thread::id &threadId

                        )
                            { // line 819
                            std::stringstream stringStreamOutput1;
                            stringStreamOutput1 << "Node with id: " << threadId << " has 'decided' to 'crash'... joining it..." << std::endl;
                            const std::string strOutput1 = stringStreamOutput1.str();
                            BZRootFrame::pushMessage("log",strOutput1.c_str());

//                            std::unique_lock<std::mutex> lockStdOut = KeplerApplication::getStdOutLock();
//                            std::cout << strOutput1;
//                            lockStdOut.unlock();

                            if(mapThreads.at(threadId).m_ptr_thread->joinable())
                                {
                                mapThreads.at(threadId).m_ptr_thread->join();
                                }

                            std::stringstream stringStreamOutput2;
                            stringStreamOutput2 << "Node with id: " << threadId << " joining DONE" << std::endl;
                            const std::string strOutput2 = stringStreamOutput2.str();
                            BZRootFrame::pushMessage("log",strOutput2.c_str());

//                            lockStdOut.lock();
//                            std::cout << strOutput2;
//                            lockStdOut.unlock();

                            mapThreads.erase(threadId);
                            newlyKilledThreadIDs.push_back(threadId);
                            }
                );
                }
    ); // end of safe use Line853

    ThreadManager::s_threadIdsToKill.safeUse( // 858
            [
                    &newlyKilledThreadIDs

            ](
                    BZSyncSetWrapper<std::thread::id>::BZSyncSetType &threadsToKill
            )
                {
                for (const auto killedThreadID :newlyKilledThreadIDs)
                    {
                    threadsToKill.erase(killedThreadID);
                    }
                }
    ); // end of safeUse

    for (const auto &threadId : newlyKilledThreadIDs) // line 868
        {
        BZRootFrame::GetInstance()->RemoveNodeListItem(threadId);
        }
}

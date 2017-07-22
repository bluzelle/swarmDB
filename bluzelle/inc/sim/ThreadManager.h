#ifndef KEPLER_THREADMANAGER_H
#define KEPLER_THREADMANAGER_H

#include "BZDataSource.h"
#include "BZSyncMapWrapper.hpp"
#include "BZSyncSetWrapper.hpp"
#include "ThreadData.h"


#include <memory>
#include <mutex>
#include <vector>
#include <string>
#include <thread>


class ThreadManager :public BZDataSource {
public:
    virtual ~ThreadManager() = default;
    static ThreadManager& GetInstance();
    std::vector<std::string> items;
    std::vector<std::string> GetData() override;
    void createNewThreadsIfNeeded();
    void killAndJoinThreadsIfNeeded();
    static bool _endAllThreads;

    // TODO: Move this to a model class
    static BZSyncMapWrapper<std::thread::id, ThreadData> s_threads;
    static BZSyncSetWrapper<std::thread::id> s_threadIdsToKill;

private:
    static std::unique_ptr<ThreadManager> m_instance;
    static std::once_flag m_onceFlag;
    ThreadManager() = default;
    ThreadManager(const ThreadManager& src) = delete;
    ThreadManager&operator=(const ThreadManager& rhs) = delete;
};


#endif //KEPLER_THREADMANAGER_H

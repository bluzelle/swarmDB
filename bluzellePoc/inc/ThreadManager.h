#ifndef KEPLER_THREADMANAGER_H
#define KEPLER_THREADMANAGER_H

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include <thread>
#include <mutex>
#include <queue>


class ThreadManager {
public:
    // thread related
    void createNewThreadsIfNeeded();
    void killAndJoinThreadsIfNeeded();
    void onNewlyCreatedThread(const std::thread::id &idNewlyCreatedThread);
    void onNewlyKilledThread(const std::thread::id &idNewlyKilledThread);
    void removeThreadIdFromListViewNodes(const std::thread::id &idThreadToRemove);

    wxTimer m_timerIdle;
    std::mutex m_textCtrlApplicationWideLogQueueMutex;
    std::queue<std::string> m_queueTextCtrlApplicationWideLog;
    unsigned int m_uintTimerCounter;
    unsigned int m_uintIdleCounter;
    unsigned int m_uintListCtrlApplicationWideLogCounter;
};



#endif //KEPLER_THREADMANAGER_H

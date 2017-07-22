#include <boost/locale.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include "KeplerApplication.hpp"
#include "KeplerFrame.hpp"
#include "ThreadData.hpp"
#include "ThreadUtilities.h"

std::mutex KeplerApplication::s_stdOutMutex;

KeplerSynchronizedMapWrapper<std::thread::id, ThreadData> KeplerApplication::s_threads;
KeplerSynchronizedSetWrapper<std::thread::id> KeplerApplication::s_threadIdsToKill;

bool KeplerApplication::s_bool_endAllThreads = false;

unsigned int KeplerApplication::sleepRandomMilliseconds(const unsigned int uintMaximumMilliseconds)
{
    int intRandomVariable = getThreadFriendlyLargeRandomNumber();
    unsigned int uintActualMillisecondsToSleep = intRandomVariable % uintMaximumMilliseconds;
    std::this_thread::sleep_for(std::chrono::milliseconds(uintActualMillisecondsToSleep));
    return uintActualMillisecondsToSleep;
}

bool KeplerApplication::OnInit()
{
    KeplerFrame *frame = new KeplerFrame();

    frame->SetSize(1000.0,800.0);
    frame->Show(true);
    return true;
}

std::unique_lock<std::mutex> KeplerApplication::getStdOutLock()
{
    std::unique_lock<std::mutex> mutexLock(
            s_stdOutMutex,
            std::defer_lock);
    mutexLock.lock();
    return mutexLock;
}

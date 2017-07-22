#ifndef KEPLER_APPLICATION_H
#define KEPLER_APPLICATION_H



#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include <iostream>
#include <vector>
#include <thread>
#include <mutex>

#include "KeplerSynchronizedSetWrapper.hpp"
#include "KeplerSynchronizedMapWrapper.hpp"



class KeplerApplication: public wxApp
    {
    public:

        virtual bool OnInit();

        static std::unique_lock<std::mutex> getStdOutLock();

        static unsigned int sleepRandomMilliseconds(const unsigned int uintMaximumMilliseconds);

        static KeplerSynchronizedMapWrapper<std::thread::id, ThreadData> s_threads;
        static KeplerSynchronizedSetWrapper<std::thread::id> s_threadIdsToKill;

        static std::mutex s_stdOutMutex;              

        static bool s_bool_endAllThreads;
    };



#endif // KEPLER_APPLICATION_H
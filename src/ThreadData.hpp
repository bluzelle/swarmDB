#ifndef THREAD_DATA_H
#define THREAD_DATA_H



#include <iostream>
#include <vector>
#include <thread>



class ThreadData
    {
    public:

        ThreadData(std::shared_ptr<std::thread> ptr_thread)
            {
            m_ptr_thread = ptr_thread;     
            }

        ThreadData(const ThreadData &original)
            {
            m_ptr_thread = original.m_ptr_thread;    

            m_vectorLogMessages = original.m_vectorLogMessages;
            }

        std::shared_ptr<std::thread> m_ptr_thread;
        std::vector<std::string> m_vectorLogMessages;
    };



#endif // THREAD_DATA_H
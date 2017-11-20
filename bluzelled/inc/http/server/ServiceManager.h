#ifndef KEPLER_SERVICEMANAGER_H
#define KEPLER_SERVICEMANAGER_H

#include "server.hpp"

#include <boost/thread.hpp>

class ServiceManager
{
    http::server::server *s_;
    boost::thread *thread_;
public:
    ServiceManager
            (
            const char *ip_address,
            const char *port,
            const char *doc_root)
    {
        s_ = new http::server::server(ip_address, port, doc_root);
    }

    void run_async()
    {
        thread_ = new boost::thread([this]()
                                        {
                                        this->s_->run();
                                        });
    }

    ~ServiceManager() {
        if (thread_ && thread_->joinable())
            {
            thread_->join();
            delete thread_;
            }
        delete s_;
    }
};
#endif //KEPLER_SERVICEMANAGER_H

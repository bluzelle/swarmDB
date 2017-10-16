#ifndef KEPLER_SERVICES_H
#define KEPLER_SERVICES_H

#include "services/Service.h"

#include <string>
#include <map>

#define kNO_SERVICE_ERROR_MSG  "{\"error\":\"Service not found.\"}"


class Services
{
    // TODO:  make Service* a unique or shared ptr...
    std::map<std::string, Service*> _services;
public:
    void add_service(std::string service_name, Service* service)
    {
        _services[service_name] = service;
    }

    std::string operator()(const std::string &service_name, const std::string& json_string)
    {
        if(_services.find(service_name) == _services.end())
            {
            return kNO_SERVICE_ERROR_MSG;
            }
        Service* service = _services[service_name];
        return (*service)(json_string);
    }

    ~Services()
    {
        // clean up the map...
    }

};

#endif //KEPLER_SERVICES_H

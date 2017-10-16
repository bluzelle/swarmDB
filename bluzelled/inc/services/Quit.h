#ifndef KEPLER_QUIT_H
#define KEPLER_QUIT_H

#include "services/Service.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <sstream>

namespace pt = boost::property_tree;

class Quit : public Service
{
public:
    std::string operator()(const std::string& json_string) override
    {

    }
};

#endif //KEPLER_QUIT_H

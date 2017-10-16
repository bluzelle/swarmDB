#ifndef KEPLER_PING_H
#define KEPLER_PING_H

#include "services/Service.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <sstream>

namespace pt = boost::property_tree;

class Ping : public Service
{
public:
    std::string operator()(const std::string& json_string) override
    {
        pt::ptree tree;
        tree = parse_input(json_string);
        tree.put("cmd","pong");
        std::stringstream ss;
        pt::write_json(ss,tree);
        return ss.str();
    }
};

#endif //KEPLER_PING_H

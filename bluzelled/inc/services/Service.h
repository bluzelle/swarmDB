#ifndef KEPLER_SERVICE_H
#define KEPLER_SERVICE_H

#include <iostream>
#include <string>
#include <boost/regex.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

namespace pt = boost::property_tree;

class Service {
public:
    virtual std::string operator()(const std::string& json_string)
    {
        std::cout<< "Service base data:[" << json_string <<"]\n";
        return "Service::Base";
    }

    pt::ptree parse_input(const std::string& json_str)
    {
        pt::ptree in_tree;
        std::stringstream ss;
        ss << json_str;
        pt::read_json(ss, in_tree);\
        return in_tree;
    }

    std::string fix_json_numbers(const std::string &json_str)
    {
        boost::regex re("\\\"([0-9]+\\.{0,1}[0-9]*)\\\"");
        return  boost::regex_replace(json_str, re, "$1");
    }
};

#endif //KEPLER_SERVICE_H

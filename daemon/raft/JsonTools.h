#ifndef BLUZELLE_JSONTOOLS_HPP_H
#define BLUZELLE_JSONTOOLS_HPP_H

#include <iostream>
#include <string>

using std::string;

#include <boost/property_tree/json_parser.hpp>

boost::property_tree::ptree pt_from_json_string(const string &s);

string pt_to_json_string(boost::property_tree::ptree pt);

#endif //BLUZELLE_JSONTOOLS_HPP_H

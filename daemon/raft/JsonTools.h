#ifndef BLUZELLE_JSONTOOLS_HPP_H
#define BLUZELLE_JSONTOOLS_HPP_H

#include <string>
#include <boost/property_tree/json_parser.hpp>

using namespace std;
namespace bpt = boost::property_tree;

bpt::ptree
pt_from_json_string(
    const string &s
);

string
pt_to_json_string(
    bpt::ptree pt
);

#endif //BLUZELLE_JSONTOOLS_HPP_H

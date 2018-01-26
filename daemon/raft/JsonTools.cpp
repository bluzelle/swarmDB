#include "JsonTools.h"

#include <iostream>
#include <sstream>

using namespace std;
namespace bpt = boost::property_tree;

bpt::ptree
pt_from_json_string(
    const string &s
)
{
    stringstream ss;
    ss << s;

    try
        {
        bpt::ptree json;
        bpt::read_json(ss, json);
        return json;
        }
    catch (bpt::json_parser_error &ex)
        {
        cerr << "Raft::from_json_string() threw '" << ex.what() << "' parsing " << std::endl;
        cerr << s << std::endl;
        }
    return bpt::ptree();
}

string
pt_to_json_string(
    bpt::ptree pt
)
{
    try
        {
        stringstream ss;
        bpt::write_json(ss, pt);
        return ss.str();
        }
    catch (bpt::json_parser_error &ex)
        {
        cerr << "Raft::to_json_string() threw '" << ex.what() << std::endl;
        }
    return string();
}

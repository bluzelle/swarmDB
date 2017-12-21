#include "JsonTools.h"

boost::property_tree::ptree pt_from_json_string(const string &s) {
    std::stringstream ss;
    ss << s;

    try
        {
        boost::property_tree::ptree json;
        boost::property_tree::read_json(ss, json);
        return json;
        }
    catch (boost::property_tree::json_parser_error &ex)
        {
        std::cout << "Raft::from_json_string() threw '" << ex.what() << "' parsing " << std::endl;
        std::cout << s << std::endl;
        }

    return boost::property_tree::ptree();
}

string pt_to_json_string(boost::property_tree::ptree pt) {
    try
        {
        std::stringstream ss;
        boost::property_tree::write_json(ss, pt);
        return ss.str();
        }
    catch (boost::property_tree::json_parser_error &ex)
        {
        std::cout << "Raft::to_json_string() threw '" << ex.what() << std::endl;
        }

    return string();
}

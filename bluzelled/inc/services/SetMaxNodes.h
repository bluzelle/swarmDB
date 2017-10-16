#ifndef KEPLER_SETMAXNODES_H
#define KEPLER_SETMAXNODES_H

#include "services/Service.h"
#include "Node.h"

#include <sstream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

namespace pt = boost::property_tree;

void set_max_nodes(unsigned long max);

class SetMaxNodes : public Service {

    pt::ptree nodes_to_tree(long seq, long new_max_nodes)
    {
        pt::ptree out_tree;
        set_max_nodes(new_max_nodes);
        out_tree.put<long>("newMaxNodes",new_max_nodes);
        out_tree.put<long>("seq", seq);
        return out_tree;
    }

    std::string tree_to_response(const pt::ptree& out_tree)
    {
        std::stringstream ss;
        ss.str("");
        pt::write_json(ss,out_tree);
        return ss.str();
    }

public:

    std::string operator()(const std::string& request) override
    {
        pt::ptree out_tree;
        try
            {
            pt::ptree in_tree;
            in_tree = parse_input(request);
            out_tree = nodes_to_tree(in_tree.get<long>("seq"),in_tree.get<long>("data"));
            }
        catch(std::exception& e)
            {
            out_tree.put("error", e.what());
            std::cerr << e.what();
            }
        return tree_to_response(out_tree);
    }
};




#endif //KEPLER_SETMAXNODES_H

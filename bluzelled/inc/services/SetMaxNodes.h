#ifndef KEPLER_SETMAXNODES_H
#define KEPLER_SETMAXNODES_H

#include "services/Service.h"
#include "NodeManager.h"

#include <sstream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

namespace pt = boost::property_tree;

class SetMaxNodes : public Service {
    std::weak_ptr<NodeManager> wk_node_manager_;

    pt::ptree nodes_to_tree(long seq, long new_max_nodes)
    {
        pt::ptree out_tree;
        std::shared_ptr<NodeManager> node_manager = wk_node_manager_.lock();
        try
            {
            node_manager->set_max_nodes(new_max_nodes);
            out_tree.put<long>("newMaxNodes",new_max_nodes);
            }
        catch(std::runtime_error &e)
            {
            std::cerr << "SetMaxNodes runtime error:" << e.what() << std::endl;
            }
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

    SetMaxNodes(const std::shared_ptr<NodeManager>& node_manager) : wk_node_manager_(node_manager)
    {}

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

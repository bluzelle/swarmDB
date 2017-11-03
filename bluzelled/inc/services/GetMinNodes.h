#ifndef KEPLER_GETMINNODES_H
#define KEPLER_GETMINNODES_H

#include "services/Service.h"
#include "NodeManager.h"

#include <sstream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

namespace pt = boost::property_tree;

class GetMinNodes : public Service
{
    std::weak_ptr<NodeManager> wk_node_manager;

    pt::ptree nodes_to_tree(long seq, unsigned long new_min_nodes)
    {
        pt::ptree out_tree;
        try
            {
            std::shared_ptr<NodeManager> node_manager = wk_node_manager.lock();
            node_manager->set_min_nodes(new_min_nodes);
            out_tree.put<long>("getMinNodes", new_min_nodes);
            }
        catch (std::runtime_error &e)
            {
            out_tree.put("error", e.what());
            std::cerr << e.what();
            }

        out_tree.put<long>("seq", seq);
        return out_tree;
    }

    std::string tree_to_response(const pt::ptree &out_tree)
    {
        auto r = boost::format("{\"getMinNodes\": %d, \"seq\": %d}")
                 % out_tree.get<long>("getMinNodes") % out_tree.get<long>("seq");

        return boost::str(r);
    }

public:

    GetMinNodes(const std::shared_ptr<NodeManager> &node_manager) : wk_node_manager(node_manager)
    {}

    std::string operator()(const std::string &request) override
    {
        // getMinNodes -> setMinNodes
        pt::ptree out_tree;
        pt::ptree in_tree;
        try
            {
            in_tree = parse_input(request);
            out_tree = nodes_to_tree(in_tree.get<long>("seq"), 0);
            }
        catch (std::exception &e)
            {
            out_tree.put("error", e.what());
            std::cerr << e.what();
            }
        return tree_to_response(out_tree);
    }
};


#endif //KEPLER_GETMINNODES_H

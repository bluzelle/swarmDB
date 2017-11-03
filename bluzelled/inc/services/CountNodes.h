#ifndef KEPLER_COUNTNODES_H
#define KEPLER_COUNTNODES_H

#include "services/Service.h"
#include "NodeManager.h"

#include <sstream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

namespace pt = boost::property_tree;

class CountNodes : public Service
{
    std::weak_ptr<NodeManager> wk_node_manager_;

    pt::ptree nodes_to_tree(long seq = 0)
    {
        pt::ptree out_tree;
        out_tree.put("cmd", "nodeCount");
        pt::ptree array;
        std::shared_ptr<NodeManager> node_manager = wk_node_manager_.lock();
        if(nullptr != node_manager)
            {
            out_tree.put<long>("count", node_manager->nodes_count());
            }
        else
            {
            out_tree.put<std::string>("error", "Unable to get the node count.");
            }
        out_tree.put<long>("seq", seq);
        return out_tree;
    }

    std::string tree_to_response(const pt::ptree &out_tree)
    {
        std::stringstream ss;
        ss.str("");
        pt::write_json(ss, out_tree);
        return ss.str();
    }

public:
    CountNodes
            (
                    const std::shared_ptr<NodeManager> &node_manager
            ) : wk_node_manager_(node_manager)
    {
    }

    std::string operator()(const std::string &request) override
    {
        pt::ptree out_tree;
        try
            {
            pt::ptree in_tree;
            in_tree = parse_input(request);
            out_tree = nodes_to_tree(in_tree.get<long>("seq"));
            }
        catch (std::exception &e)
            {
            out_tree.put("error", e.what());
            std::cerr << e.what();
            }
        return fix_json_numbers(tree_to_response(out_tree));
    }
};

#endif //KEPLER_COUNTNODES_H

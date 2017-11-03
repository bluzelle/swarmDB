#ifndef KEPLER_GETALLNODES_H
#define KEPLER_GETALLNODES_H

#include "services/Service.h"
#include "Node.h"
#include "NodeManager.h"

#include <sstream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

namespace pt = boost::property_tree;

class GetAllNodes : public Service
{
    std::weak_ptr<NodeManager> wk_node_manager_;
    system_clock::time_point last_update_;

    const pt::ptree nodes_to_tree(const std::shared_ptr<NodeManager> &nm)
    {
        pt::ptree array;
        for (auto node : nm->nodes())
            {
            pt::ptree child1;
            child1.put("address", node->name());
            child1.put("nodeState", node->state());
            child1.put<long>("messages", node->state());
            array.push_back(std::make_pair("", child1));
            }
        return array;
    }

    pt::ptree compose_response(long seq = 0)
    {
        pt::ptree out_tree;
        out_tree.put("cmd", "updateNodes");

        std::shared_ptr<NodeManager> nm = wk_node_manager_.lock();
        if (nullptr != nm)
            {
            pt::ptree array = nodes_to_tree(nm);
            out_tree.add_child("data", array);
            }
        else
            {
            out_tree.put("error", "Unable to access the node manager.");
            }

        out_tree.put("seq", seq);
        last_update_ = system_clock::now();
        return out_tree;
    }

    std::string tree_to_response(const pt::ptree &out_tree)
    {
        std::stringstream ss;
        ss.str("");
        auto d = out_tree.get_child("data.");
        if (!d.empty())
            {
            pt::write_json(ss, out_tree);
            }

        return ss.str();
    }

public:
    GetAllNodes
            (
                    const std::shared_ptr<NodeManager> &node_manager
            ) : wk_node_manager_(node_manager)
    {
    }

    std::string operator()(const std::string &request) override
    {
        pt::ptree out_tree;
        pt::ptree in_tree;
        in_tree = parse_input(request);
        out_tree = compose_response(in_tree.get<long>("seq"));
        return fix_json_numbers(tree_to_response(out_tree));
    }
};

#endif //KEPLER_GETALLNODES_H

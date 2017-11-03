#ifndef KEPLER_GETMAXNODES_H
#define KEPLER_GETMAXNODES_H

#include "services/Service.h"
#include "NodeManager.h"

#include <sstream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

namespace pt = boost::property_tree;

unsigned long get_max_nodes();

class GetMaxNodes : public Service
{
    std::weak_ptr<NodeManager> wk_node_manager_;

    pt::ptree nodes_to_tree(long seq, long max_nodes)
    {
        pt::ptree out_tree;
        out_tree.put<long>("getMaxNodes", max_nodes);
        out_tree.put<long>("seq", seq);
        return out_tree;
    }

    std::string tree_to_response(const pt::ptree &out_tree)
    {
        auto r = boost::format("{\"getMaxNodes\": %d, \"seq\": %d}")
                 % out_tree.get<long>("getMaxNodes") % out_tree.get<long>("seq");

        return boost::str(r);
    }

public:

    GetMaxNodes(const std::shared_ptr<NodeManager> &node_manager) : wk_node_manager_(node_manager)
    {}

    std::string operator()(const std::string &request) override
    {
        // getMaxNodes -> setMaxNodes
        pt::ptree out_tree;
        pt::ptree in_tree;
        size_t max_nodes = 0;
        try
            {
            in_tree = parse_input(request);
            std::shared_ptr<NodeManager> nm = wk_node_manager_.lock();
            out_tree.put("cmd", "setMaxNodes");

            if (nullptr != nm)
                {
                max_nodes = nm->max_nodes();
                }
            else
                {
                out_tree.put("error", "Unable to access node manager.");
                }
            }
        catch (std::runtime_error &e)
            {
            out_tree.put("error", e.what());
            std::cerr << e.what();

            }
        catch (std::exception &e)
            {
            out_tree.put("error", e.what());
            std::cerr << e.what();
            }

        out_tree = nodes_to_tree(in_tree.get<long>("seq"), max_nodes);
        return tree_to_response(out_tree);
    }
};

#endif //KEPLER_GETMAXNODES_H

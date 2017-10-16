#ifndef KEPLER_GETALLNODES_H
#define KEPLER_GETALLNODES_H

#include "services/Service.h"
#include "Node.h"

#include <sstream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

namespace pt = boost::property_tree;

class GetAllNodes : public Service {
protected:
    Nodes *nodes_;
    system_clock::time_point last_update_;

    pt::ptree nodes_to_tree(long seq = 0) {
        pt::ptree out_tree;
        out_tree.put("cmd", "updateNodes");

        pt::ptree array;
        for (auto node : *nodes_)
            {
            //if (node->last_change() > last_update_)
                {
                pt::ptree child1;
                child1.put("address", node->name());
                child1.put("nodeState", node->state());
                child1.put<long>("messages", node->state());
                array.push_back(std::make_pair("", child1));
                }
            }
        out_tree.add_child("data", array);
        out_tree.put("seq", seq);

        last_update_ = system_clock::now();

        return out_tree;
    }

    std::string tree_to_response(const pt::ptree &out_tree) {
        std::stringstream ss;
        ss.str("");
        auto d = out_tree.get_child("data.");
        if (d.size() != 0)
            {
            pt::write_json(ss, out_tree);
            }

        return ss.str();
    }

public:
    GetAllNodes(Nodes *nodes) : nodes_(nodes) {
        //last_update_ = system_clock::now();
    }

    std::string operator()(const std::string &request) override {
        pt::ptree out_tree;
        pt::ptree in_tree;

        in_tree = parse_input(request);
        out_tree = nodes_to_tree(in_tree.get<long>("seq"));

        return fix_json_numbers(tree_to_response(out_tree));
    }

    void set_nodes(Nodes *nodes) { nodes_ = nodes; }
};

#endif //KEPLER_GETALLNODES_H

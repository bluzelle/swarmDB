#ifndef KEPLER_REMOVENODES_H
#define KEPLER_REMOVENODES_H

#include <sstream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "services/Service.h"

class RemoveNodes : public Service {
    std::string name_;

public:
    RemoveNodes(const std::string &name) : name_(name) {

    }

    pt::ptree nodes_to_tree(long seq = 0) {
        pt::ptree out_tree;
        out_tree.put("cmd", "removeNodes");

        pt::ptree array;
        pt::ptree child1;
        child1.put("address", name_);
        array.push_back(std::make_pair("", child1));
        out_tree.add_child("data", array);
        out_tree.put("seq", seq);

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

    std::string operator()(const std::string &request) override {
        pt::ptree out_tree;
        pt::ptree in_tree;

        in_tree = parse_input(request);
        out_tree = nodes_to_tree(in_tree.get<long>("seq"));

        return tree_to_response(out_tree);
    }
};

#endif //KEPLER_REMOVENODES_H

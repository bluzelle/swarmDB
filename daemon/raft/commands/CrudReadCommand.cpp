#include "CrudReadCommand.h"

CrudReadCommand::CrudReadCommand(Storage& s, string k)
        : storage_(s), key_(std::move(k)) {

}

boost::property_tree::ptree CrudReadCommand::operator()() {
        boost::property_tree::ptree result;

        return result;
}
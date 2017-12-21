#include "CrudCreateCommand.h"

#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/string_generator.hpp>

CrudCreateCommand::CrudCreateCommand(Storage& s, string k, string v)
    : storage_(s), key_(std::move(k)), value_(std::move(v)) {

}

boost::property_tree::ptree CrudCreateCommand::operator()()
{
    static boost::mt19937 ran;
    static boost::uuids::basic_random_generator<boost::mt19937> gen(&ran);
    storage_.create(key_, value_, boost::uuids::to_string(gen()));
    return success();
}

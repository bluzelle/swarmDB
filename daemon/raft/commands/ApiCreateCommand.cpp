#include <boost/format.hpp>
#include <boost/uuid/uuid_generators.hpp>

#include "ApiCreateCommand.h"
#include "JsonTools.h"

ApiCreateCommand::ApiCreateCommand(
    ApiCommandQueue& q,
    Storage& s,
    boost::property_tree::ptree pt)
        : queue_(q), storage_(s), pt_(pt)
{
}

boost::property_tree::ptree ApiCreateCommand::operator()()
{
    auto data  = pt_.get_child("data.");

    string key;
    if (data.count("key") > 0)
        key = data.get<string>("key");

    string val;
    if (data.count("value") > 0)
        val = data.get<string>("value");

    if (!key.empty())
        {
        // Store locally.
        static boost::uuids::random_generator uuid_gen;
        boost::uuids::uuid transaction_id = uuid_gen();
        storage_.create(
                key,
                val,
                boost::uuids::to_string(transaction_id)
        );

        // {"bzn-api":"create", "transaction-id":"123", "data":{key":"key_one", "value":"value_one"}}
        // {"crud":"create", "transaction-id":"123", "data":{key":"key_one", "value":"value_one"}}
        string resp = pt_to_json_string(pt_);
        resp.replace(resp.find("bzn-api"), 7, "crud");

        queue_.push(
            std::make_pair<string,string>(
                pt_.get<string>("transaction-id"),
                std::move(resp)
            )
        );

        return success();
        }

    return error("key is missing");
}
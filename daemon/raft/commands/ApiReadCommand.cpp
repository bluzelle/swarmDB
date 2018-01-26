#include "ApiReadCommand.h"

#include <algorithm>
#include <boost/format.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/string_generator.hpp>


ApiReadCommand::ApiReadCommand(
        ApiCommandQueue& q,
        Storage& s,
        boost::property_tree::ptree pt
)
        : queue_(q), storage_(s), pt_(pt)
{
    cerr << "Where is ApiCommandQueue [" << &queue_ << "] used?\n";

}

boost::property_tree::ptree ApiReadCommand::operator()()
{
    auto data  = pt_.get_child("data.");

    string key;
    if (data.count("key") > 0)
        {
        key = data.get<string>("key");
        }

    if (!key.empty())
        {
        Record val = storage_.read(key); // Read from local storage, no need to send to followers.
        // TODO: Dmitry, here is a place we need to do some refactoring. Record doesn't hold value as a string, it's a blob.
        // TODO: This is a bad thing to do:
        string val_str;
        val_str.reserve(
                val.value_.size()
        );
        std::transform(
                val.value_.begin(),
                val.value_.end(),
                std::back_inserter(val_str),
                [](auto c)
                    {
                    return (char)c;
                    }
        );
        return result(val_str);
        }

    return error("key/value pair is missing");
}
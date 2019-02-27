// Copyright (C) 2018 Bluzelle
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License, version 3,
// as published by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#include <pbft/pbft_persistent_state.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <pbft/operations/pbft_operation.hpp>

using namespace bzn;

std::set<std::string> persist_base::initialized_containers;

std::string
persist_base::escape(const std::string& input)
{
    std::string output;

    for (auto ch : input)
    {
        output += (ch == ESCAPE_1) ? std::string{ESCAPE_1} + ESCAPE_2 : std::string{ch};
    }

    return output;
}

std::string
persist_base::unescape(const std::string& input)
{
    std::string output;

    auto it = input.begin();
    while (it != input.end())
    {
        if (*it == ESCAPE_1)
        {
            if (it + 1 != input.end() && *(it + 1) == ESCAPE_2)
            {
                output += *it;
                it += 2;
            }
            else
            {
                // a bare ESCAPE_1 character was found, which should never happen.
                // if you hit this you've likely specified a key incorrectly
                LOG(error) << "illegal character unescaping key for persistent value";
                throw std::runtime_error("illegal character unescaping key for persistent value");
            }
        }
        else
        {
            output += *it++;
        }
    }

    return output;
}

template<>
std::string
persistent<std::string>::to_string(const std::string &value)
{
    return value;
}

template<>
std::string
persistent<std::string>::from_string(const std::string &value)
{
    return value;
}

template<>
std::string
persistent<bzn::log_key_t>::to_string(const bzn::log_key_t &key)
{
    return (boost::format("%020u_%020u") % std::get<0>(key) % std::get<1>(key)).str();
}

template<>
bzn::log_key_t
persistent<bzn::log_key_t>::from_string(const std::string &value)
{
    auto offset = value.find('_');
    if (offset < value.size())
    {
        try
        {
            uint64_t v0 = boost::lexical_cast<uint64_t>(value.substr(0, offset).c_str());
            uint64_t v1 = boost::lexical_cast<uint64_t>(value.substr(offset + 1).c_str());
            if (v0 && v1)
            {
                return log_key_t{v0, v1};
            }
        }
        catch (boost::bad_lexical_cast &)
        {
            LOG(error) << "Error converting log key from persistent state";
        }
    }

    LOG(error) << "bad log key from persistent state";
    throw std::runtime_error("bad log key from persistent state");
}

template<>
std::string
persistent<bzn::operation_key_t>::to_string(const bzn::operation_key_t &key)
{
    return (boost::format("%020u_%020u_%s") % std::get<0>(key) % std::get<1>(key) % std::get<2>(key)).str();
}

template<>
bzn::operation_key_t
persistent<bzn::operation_key_t>::from_string(const std::string &value)
{
    auto offset1 = value.find('_');
    if (offset1 < value.size())
    {
        auto offset2 = value.find('_', offset1 + 1);
        if (offset2 < value.size())
        {
            try
            {
                uint64_t v0 = boost::lexical_cast<uint64_t>(value.substr(0, offset1).c_str());
                uint64_t v1 = boost::lexical_cast<uint64_t>(
                    value.substr(offset1 + 1, offset2 - (offset1 + 1)).c_str());
                if (v0 && v1 && !value.substr(offset2 + 1).empty())
                {
                    return operation_key_t{v0, v1, value.substr(offset2 + 1)};
                }
            }
            catch (boost::bad_lexical_cast &)
            {
                LOG(warning) << "Error converting operation key";
            }
        }
    }

    LOG(error) << "bad log key from persistent state";
    throw std::runtime_error("bad log key from persistent state");
}

template<>
std::string
persistent<bzn::checkpoint_t>::to_string(const bzn::checkpoint_t &cp)
{
    return (boost::format("%020u_%s") % cp.first % cp.second).str();
}

template<>
bzn::checkpoint_t
persistent<bzn::checkpoint_t>::from_string(const std::string &value)
{
    auto offset = value.find('_');
    if (offset < value.size())
    {
        try
        {
            uint64_t v0 = boost::lexical_cast<uint64_t>(value.substr(0, offset).c_str());

            // TODO: checkpoint hashes are currently empty. remove comments after they're done (KEP-1203)
//            if (!value.substr(offset + 1).empty())
//            {
                return checkpoint_t{v0, value.substr(offset + 1)};
//            }
        }
        catch (boost::bad_lexical_cast &)
        {
            LOG(warning) << "Error converting checkpoint";
        }
    }

    LOG(error) << "bad checkpoint from persistent state";
    throw std::runtime_error("bad checkpoint from persistent state");
}

template <>
std::string
persistent<uint64_t>::to_string(const uint64_t& val)
{
    return (boost::format("%020u") % val).str();
}

template <>
uint64_t
persistent<uint64_t>::from_string(const std::string& value)
{
    try
    {
        return boost::lexical_cast<uint64_t>(value);
    }
    catch (boost::bad_lexical_cast &)
    {
    }

    LOG(error) << "bad uint64_t from persistent state";
    throw std::runtime_error("bad uint64_t from persistent state");
}

template<>
std::string
persistent<bool>::to_string(const bool &val)
{
    return (boost::format("%01u") % val).str();
}

template<>
bool
persistent<bool>::from_string(const std::string &value)
{
    try
    {
        return boost::lexical_cast<bool>(value);
    }
    catch (boost::bad_lexical_cast &)
    {
    }

    LOG(error) << "bad bool from persistent state";
    throw std::runtime_error("bad bool from persistent state");
}

template<>
std::string
persistent<bzn_envelope>::to_string(const bzn_envelope &val)
{
    return val.SerializeAsString();
}

template<>
bzn_envelope
persistent<bzn_envelope>::from_string(const std::string &value)
{
    bzn_envelope env;
    if (env.ParseFromString(value))
    {
        return env;
    }

    LOG(error) << "bad bzn_envelope from persistent state";
    throw std::runtime_error("bad bzn_envelope from persistent state");
}

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

#include <include/bluzelle.hpp>
#include <utils/make_endpoint.hpp>


std::optional<boost::asio::ip::tcp::endpoint>
bzn::make_endpoint(const std::string& host, const std::string& port)
{
    using namespace boost::asio;

    io_context ioc;
    ip::tcp::resolver resolver{ioc};

    try
    {
        ip::tcp::resolver::query query(ip::tcp::v4(), host, port);

        auto endpoint_iterator = resolver.resolve(query);

        if (!endpoint_iterator.empty())
        {
            return *endpoint_iterator;
        }
    }
    catch(std::exception& ex)
    {
        LOG(error) << ex.what();
    }

    LOG(error) << "Could not resolve: " << host;

    return std::nullopt;
}


std::optional<boost::asio::ip::tcp::endpoint>
bzn::make_endpoint(const bzn::peer_address_t& peer)
{
    return bzn::make_endpoint(peer.host, std::to_string(peer.port));
}

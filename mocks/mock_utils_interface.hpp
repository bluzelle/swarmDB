#pragma once
#include <utils/utils_interface.hpp>

namespace bzn {

    class mock_utils_interface_base : public utils_interface_base {
    public:
        MOCK_CONST_METHOD3(get_peer_ids,
                std::vector<std::string>(const bzn::uuid_t& swarm_id, const std::string& esr_address, const std::string& url));
        MOCK_CONST_METHOD4(get_peer_info,
                bzn::peer_address_t(const bzn::uuid_t& swarm_id, const std::string& peer_id, const std::string& esr_address, const std::string& url));
        MOCK_CONST_METHOD2(sync_req,
                std::string(std::string, std::string));
    };

}  // namespace bzn

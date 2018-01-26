#include "CommandFactory.h"
#include "PeerList.h"
//#include "Storage.h"
//#include "ApiCommandQueue.h"
//#include "Command.hpp"
#include "Raft.h"

#include <memory>
#include <sstream>
#include <boost/test/unit_test.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/asio.hpp>

struct F
{
    F() = default;

    ~F() = default;
};

namespace pt = boost::property_tree;
namespace asio = boost::asio;

class MockRaftState : public RaftState {
public:
    MockRaftState(
        asio::io_service& ios,
        Storage& s,
        CommandFactory& cf,
        ApiCommandQueue& pq,
        PeerList& ps,
        function<string(const string&)> rh,
        unique_ptr<RaftState>& ns)
    : RaftState(ios, s, cf, pq, ps, rh, ns)
    {
        std::cout << "I am MockRaft" << std::endl;
    }

    RaftStateType
    get_type() const
    {
        return RaftStateType::Follower;
    }

    unique_ptr<RaftState>
    handle_request(
        const string& request,
        string& response
    ) override
    {
        (void)request;
        (void)response;

        return nullptr;
    }


};

pt::ptree
make_property_tree(
    const std::string& json_command_string
)
{
    pt::ptree p_tree;
    std::stringstream ss;
    ss << json_command_string;
    pt::read_json(ss, p_tree);
    return p_tree;
}

unique_ptr<MockRaftState>
make_mock_raft_state(
    Storage& storage,
    CommandFactory& command_factory,
    ApiCommandQueue& api_command_queue
)
{
    DaemonInfo& daemon_info = DaemonInfo::get_instance();
    daemon_info.host_port() = 45000;

    asio::io_service ios;
    PeerList peer_list(ios);

    function<std::string(const std::string&)> rh = [](const std::string& sr) -> std::string
    {
        std::cout << "rh -> [" << sr << "]\n";
        return std::string("???");
    };

    MockRaftState* mrs = nullptr;
    std::unique_ptr<RaftState> umrs(mrs);



    return std::make_unique<MockRaftState>(
        ios,
        storage,
        command_factory,
        api_command_queue,
        peer_list,
        rh,
        umrs
    );
}

BOOST_FIXTURE_TEST_SUITE(command_factory_suite, F)

//    BOOST_AUTO_TEST_CASE(make_command_test)
//    {
//        Storage             storage;
//        ApiCommandQueue     api_command_queue;
//        CommandFactory      sut(storage, api_command_queue);
//        unique_ptr<MockRaftState> mock_raft_state = make_mock_raft_state(
//            storage,
//            sut,
//            api_command_queue
//        );
//
//        MockRaftState* p = mock_raft_state.get();
//
//        pt::ptree p_tree = make_property_tree(
//            R"({"cmd":"ping", "seq":123})"
//        );
//        auto command = sut.get_command(p_tree, *p);
//        BOOST_CHECK(command.get() != nullptr);
//    }


BOOST_AUTO_TEST_SUITE_END()
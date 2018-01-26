#include "CommandFactory.h"
#include "PeerList.h"
#include "RaftFollowerState.h"
#include "raft/JsonTools.h"

#include <boost/test/unit_test.hpp>

struct F
{
    F() = default;

    ~F() = default;
};


BOOST_FIXTURE_TEST_SUITE(command_factory_tests, F)

    BOOST_AUTO_TEST_CASE( test_cstor )
    {
        Storage st;
        ApiCommandQueue acq;
        CommandFactory* sut = new CommandFactory(st, acq);
        // No members to test here, just building is a good unit test.
        delete sut;
    }

    BOOST_AUTO_TEST_CASE( test_get_api_command )
    {
        try
            {
            Storage st;
            ApiCommandQueue acq;
            CommandFactory sut(st,acq);
            boost::property_tree::ptree pt = pt_from_json_string
                (
                    R"({"bzn-api":"create", "transaction-id":"123", "data":{"key":"key_222", "value":"value_222"}})"
                );

            boost::asio::io_service ios;
            PeerList pl(ios);
            unique_ptr<RaftState> rs;
            RaftFollowerState fs( ios, st, sut, acq, pl,[](const string& m)->string{ return m;}, rs);
            unique_ptr<Command> cmd = sut.get_command( pt, fs);
            boost::property_tree::ptree pt_resp = (*cmd)() ;

            BOOST_CHECK_EQUAL(pt_resp.get<string>("result"), "success");
            Record rec = st.read("key_222");

            BOOST_CHECK_EQUAL(rec.value(),"value_222");



            }
        catch(const exception& e )
            {
            cerr << "test_get_api_command error: [" << e.what() << "]\n";
            }
    }

BOOST_AUTO_TEST_SUITE_END()
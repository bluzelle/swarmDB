#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE "BaseClassModule01"

#include "Listener.h"
#include "WebSocketServer.h"

#include <boost/test/unit_test.hpp>

struct F
{
    F() = default;

    ~F() = default;
};


BOOST_FIXTURE_TEST_SUITE(base_suite, F)
    BOOST_AUTO_TEST_CASE(echo)
    {



        std::shared_ptr<Listener> listener;
        WebSocketServer web_socket_server("127.0.0.1",3000,listener,1);
        boost::thread websocket = boost::thread(web_socket_server);



    }
BOOST_AUTO_TEST_SUITE_END()
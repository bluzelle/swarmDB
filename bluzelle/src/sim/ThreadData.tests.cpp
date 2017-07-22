#include <boost/test/unit_test.hpp>
#include "ThreadData.h"


BOOST_AUTO_TEST_CASE( ThreadDataTest )
{
    std::shared_ptr<std::thread> pthread;
    ThreadData sut(pthread);
    sut.m_vectorLogMessages.emplace_back(std::string("test"));

    BOOST_CHECK(sut.m_ptr_thread == pthread);
    BOOST_CHECK(sut.m_vectorLogMessages.size() == 1);

}


BOOST_AUTO_TEST_CASE( ThreadDataTest_copy )
{
    std::shared_ptr<std::thread> pthread;
    ThreadData sut(pthread);
    ThreadData cp(sut);
    BOOST_CHECK(sut.m_ptr_thread == cp.m_ptr_thread);
}



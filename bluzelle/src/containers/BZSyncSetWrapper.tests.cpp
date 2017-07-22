#include <boost/test/unit_test.hpp>
#include "BZSyncMapWrapper.hpp"

#include "BZSyncSetWrapper.hpp"

using namespace std;

BOOST_AUTO_TEST_CASE(BZSyncSetWrapperTest_UnSafeInsert) {
    BZSyncSetWrapper<string> sut;
    string test("test");
    BOOST_CHECK(sut.unsafeInsert(test));
    BOOST_CHECK(1 == sut.Size());
}

BOOST_AUTO_TEST_CASE(BZSyncSetWrapperTest_SafeInsert) {
    BZSyncSetWrapper<string> sut;
    string test("test");
    BOOST_CHECK(sut.safeInsert(test));
    BOOST_CHECK(1 == sut.Size());

    // should not be able to insert the same thing twice
    BOOST_CHECK(false == sut.safeInsert(test));
}

class Functor0 {
    static uint8_t _count;
public:
    void operator()(string s) { ++_count; }
    static uint8_t GetCount() { return Functor0::_count; }
};

uint8_t Functor0::_count = 0;

BOOST_AUTO_TEST_CASE(BZSyncSetWrapperTest_SafeIterate) {
    BZSyncSetWrapper<string> sut;
    Functor0 f;
    string test("");
    const uint8_t expectedCount = 10;

    for (uint8_t i = 0; i < expectedCount; ++i)
        {
        test = test.append("x");
        BOOST_CHECK(sut.unsafeInsert(test));
        }
    sut.safeIterate(f);
    BOOST_CHECK(Functor0::GetCount() == expectedCount);
}


class Functor2 {
    static unsigned long _count;
public:
    void operator()(set<string> s) {
        Functor2::_count = s.size();
    }
    static unsigned long GetCount() { return Functor2::_count; }
};

unsigned long Functor2::_count = 0;

BOOST_AUTO_TEST_CASE(BZSyncSetWrapperTest_SafeUse) {
    BZSyncSetWrapper<string> sut;
    Functor2 f;
    string test("");
    const uint8_t expectedCount = 10;

    for (uint8_t i = 0; i < expectedCount; ++i)
        {
        test = test.append("x");
        BOOST_CHECK(sut.unsafeInsert(test));
        }
    sut.safeUse(f);
    BOOST_CHECK(Functor2::GetCount() == expectedCount);
}





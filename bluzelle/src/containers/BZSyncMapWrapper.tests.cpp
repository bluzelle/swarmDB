#include <boost/test/unit_test.hpp>
#include "BZSyncMapWrapper.hpp"

using namespace std;

//BOOST_AUTO_TEST_CASE( BZSyncMapWrapperTest_SafeInsert )
//{
//    BZSyncMapWrapper<int, string> sut;
//
//    int key1 = 1;
//    string val1("first");
//    BOOST_CHECK( sut.safe_insert(key1,val1) );
//}

//BOOST_AUTO_TEST_CASE( BZSyncMapWrapperTest_UnsafeInsert )
//{
//    BZSyncMapWrapper<int, string&> sut;
//    int key1 = 1;
//    std::string val1("first");
//    BOOST_CHECK( sut.unsafeInsert(key1,val1) );
//}
//
//
//class Functor {
//    static int _count ;
//public:
//    Functor(int count=0)  {
//        Functor::_count = count;
//    }
//    void operator() (int first, string second) {
//        ++Functor::_count;
//
//    };
//    static int GetCount() {return _count;}
//};
//int Functor::_count = 0;
//
//
//BOOST_AUTO_TEST_CASE( BZSyncMapWrapperTest_safeIterate )
//{
//    BZSyncMapWrapper<int,string&> sut;
//    Functor f;
//
//    // ... add map values
//    int key = 1;
//    int expectedCount = 10;
//    string value;
//
//    for( key  = 0; key < expectedCount; key++)
//        {
//        value.append("a");
//        BOOST_CHECK(sut.unsafeInsert(key,value));
//        }
//    sut.safeIterate(f);
//    BOOST_CHECK(f.GetCount() == expectedCount );
//}
//
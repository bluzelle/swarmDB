// See KEP-87
#include "Storage.h"

#include <boost/filesystem.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/string_generator.hpp>
#include <boost/test/unit_test.hpp>
#include <string>
#include <cstdlib>
#include <climits>

struct F
{
    F() = default;

    ~F() = default;
};

boost::mt19937 ran;
boost::uuids::basic_random_generator<boost::mt19937> gen(&ran);
boost::random::uniform_int_distribution<> random_unsigned_char(0,UCHAR_MAX);

const UUID_t        create_random_uuid();
const VEC_BIN_t     create_random_value(const size_t &sz = 1024);

// --run_test=storage
BOOST_FIXTURE_TEST_SUITE(storage, F)

    BOOST_AUTO_TEST_CASE( test_read_empty )
    {
        Storage         sut;
        const std::string    key{"fluffy"};
        const VEC_BIN_t accepted_value = create_random_value();

        auto record = sut.read(key);
        BOOST_CHECK( 0 == record.timestamp_ );
        BOOST_CHECK( 0 == record.value_.size() );
        BOOST_CHECK( record.transaction_id_.is_nil() );
    }

    BOOST_AUTO_TEST_CASE( test_create_read )
    {
        Storage         sut;
        std::string     key{"fluffy"};
        const VEC_BIN_t accepted_value = create_random_value(1024);
        const UUID_t    accepted_tx_id = create_random_uuid();

        sut.create(
                key,
                accepted_value,
                accepted_tx_id
        );

        auto record = sut.read(key);
        BOOST_CHECK( accepted_value == record.value_ );
        BOOST_CHECK( accepted_tx_id == record.transaction_id_ );

        const double diff = std::abs(std::difftime(time(nullptr), record.timestamp_ ));
        BOOST_CHECK( diff < 2.0 );
    }

    BOOST_AUTO_TEST_CASE( test_many_creates_reads)
    {
        std::map<std::string, Record> records;
        Storage sut;
        for(size_t i=0;i<10;++i)
        {
            const std::string key{"Fluffy"};
            const VEC_BIN_t value = create_random_value(rand() % 1024);
            const UUID_t tx_id = create_random_uuid();
            Record record{time(nullptr), value, tx_id};
            records[key] = record;

            sut.create(key, value, tx_id);
        }


        std::for_each( records.begin(), records.end(), [&sut](const auto &p)
        {
            auto accepted_key = p.first;
            auto accepted_record = p.second;
            auto record = sut.read(accepted_key);
            double diff = std::fabs(std::difftime(accepted_record.timestamp_, record.timestamp_));
            BOOST_CHECK(diff < 2.0);
            BOOST_CHECK(accepted_record.value_.size() == record.value_.size());
            BOOST_CHECK(accepted_record.value_ == record.value_);
            BOOST_CHECK(accepted_record.transaction_id_ == record.transaction_id_);
        });
    }

    BOOST_AUTO_TEST_CASE( test_update )
    {
        Storage sut;
        const std::string key{"Mr. Kitty"};
        const VEC_BIN_t accepted_value = create_random_value(1024);
        const UUID_t accepted_tx_id = create_random_uuid();
        sut.create(key, accepted_value, accepted_tx_id);

        const auto record = sut.read(key);

        BOOST_CHECK(record.value_ == accepted_value);
        BOOST_CHECK(record.transaction_id_ == accepted_tx_id);

        const VEC_BIN_t new_value = create_random_value(1024);

        sut.update( key, new_value);
        const auto updated_record = sut.read(key);
        BOOST_CHECK(updated_record.value_ == new_value);
        BOOST_CHECK(updated_record.transaction_id_ == accepted_tx_id);
    }

    BOOST_AUTO_TEST_CASE( test_remove )
    {
        Storage sut;
        const std::string key{"Mr. Kitty"};
        const VEC_BIN_t accepted_value = create_random_value(256);
        const UUID_t accepted_tx_id = create_random_uuid();

        sut.create( "akey01",create_random_value(1024), create_random_uuid());
        sut.create( "akey02",create_random_value(1024), create_random_uuid());
        sut.create( key, accepted_value, accepted_tx_id);
        sut.create( "akey04",create_random_value(1024), create_random_uuid());
        sut.create( "akey05",create_random_value(1024), create_random_uuid());

        const auto record = sut.read(key);
        BOOST_CHECK(record.value_ == accepted_value);
        BOOST_CHECK(record.transaction_id_ == accepted_tx_id);

        sut.remove(key);

        const auto x_record = sut.read(key);
        BOOST_CHECK( 0 == x_record.value_.size() );
        BOOST_CHECK(x_record.transaction_id_.is_nil());
    }

    // --run_test=storage/test_create_with_strings
    BOOST_AUTO_TEST_CASE( test_create_with_strings )
    {
        Storage sut;
        const std::string accepted_key{"Fluffy"};
        std::string accepted_value = "this is a string that should come back to us.";
        VEC_BIN_t accepted_blob_value;
        accepted_blob_value.reserve(accepted_value.size());
        for(auto c : accepted_value)
            {
            accepted_blob_value.emplace_back((unsigned char) c);
            }

        const UUID_t accepted_tx_id = create_random_uuid();
        std::string key_string{accepted_key};
        std::string tx_id_string = boost::uuids::to_string(accepted_tx_id);
        sut.create(key_string, accepted_value, tx_id_string);

        auto record = sut.read(accepted_key);
        BOOST_CHECK( accepted_blob_value == record.value_ );
        BOOST_CHECK( accepted_tx_id == record.transaction_id_ );
    }


    // TODO: Mehdi
//    BOOST_AUTO_TEST_CASE( test_storage_persistence )
//    {
//        std::string filename{"/tmp/bluzelle/persistence.txt"};
//        Storage sut(filename);
//
//    }


BOOST_AUTO_TEST_SUITE_END()

const UUID_t create_random_uuid()
{
    return gen();
}

const VEC_BIN_t create_random_value(const size_t &sz)
{
    VEC_BIN_t value;
    for(size_t i=0; i<sz ; ++i )
    {
        unsigned char x = random_unsigned_char(ran);
        value.emplace_back(x);
    }
    return value;
}

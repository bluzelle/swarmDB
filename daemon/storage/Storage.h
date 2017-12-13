#ifndef BLUZELLE_STORAGE_H
#define BLUZELLE_STORAGE_H

#include <string>
#include <vector>
#include <ctime>
#include <unordered_map>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/nil_generator.hpp>

typedef boost::uuids::uuid UUID_t;
typedef std::vector<unsigned char> VEC_BIN_t;

struct Record
{
    time_t              timestamp_;
    VEC_BIN_t  value_;
    UUID_t              transaction_id_;
};

class Storage
{
private:
    std::unordered_map<std::string, Record> kv_store_;
    static boost::uuids::nil_generator nil_uuid_gen;
    static VEC_BIN_t nil_value;

public:
    Storage();
    ~Storage();

    void create(
            const UUID_t& key,
            const VEC_BIN_t& value,
            const UUID_t& transaction_id
    );

    Record read(
            const UUID_t &key
    )
    {
        Storage::nil_value.resize(0);
        Record nil_record{ 0, Storage::nil_value, Storage::nil_uuid_gen()};
        auto rec = kv_store_.find(boost::uuids::to_string(key));
        return (rec != kv_store_.end()) ? rec->second : nil_record;
    }

    void update(
            const UUID_t& key,
            const VEC_BIN_t& value
    );

    void remove(
            const UUID_t& key
    );
};

#endif //BLUZELLE_STORAGE_H

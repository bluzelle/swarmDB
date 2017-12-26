#ifndef BLUZELLE_STORAGE_H
#define BLUZELLE_STORAGE_H

#include <iostream>
#include <string>
#include <vector>
#include <ctime>
#include <unordered_map>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/nil_generator.hpp>
#include <boost/uuid/string_generator.hpp>
#include <boost/lexical_cast.hpp>

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
    Storage() = default;
    Storage(std::string filename)
    {
        //std::cerr << "Storage does not save to: [" << filename << "] yet." << std::endl;
    }
    ~Storage() = default;

    void create(
            const std::string& key,
            const VEC_BIN_t& value,
            const UUID_t& transaction_id
    );

    void create(
            const std::string& key,
            const std::string& value,
            const std::string& transaction_id
    )
    {
        //static boost::uuids::string_generator gen;
        VEC_BIN_t blob;
        blob.reserve(value.size());
        for(auto c : value)
            {
            blob.emplace_back((unsigned char) c);
            }

        create(key,
               blob,
               boost::lexical_cast<boost::uuids::uuid>(transaction_id));
    };

    Record read(
            const std::string &key
    )
    {
        Storage::nil_value.resize(0);
        Record nil_record{ 0, Storage::nil_value, Storage::nil_uuid_gen()};
        auto rec = kv_store_.find(key);
        return (rec != kv_store_.end()) ? rec->second : nil_record;
    }

    void update(
            const std::string& key,
            const VEC_BIN_t& value
    );

    void remove(
            const std::string& key
    );
};

#endif //BLUZELLE_STORAGE_H

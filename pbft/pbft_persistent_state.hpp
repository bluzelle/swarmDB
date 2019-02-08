// Copyright (C) 2018 Bluzelle
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License, version 3,
// as published by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#pragma once

#include <storage/storage_base.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>

namespace
{
    const char SEPARATOR = '/';
    const std::string STATE_UUID{"pbftstate"};
}

namespace bzn
{
    using checkpoint_t = std::pair<uint64_t, bzn::hash_t>;

    class persist_base
    {
    protected:
        static std::string escape(const std::string& input);
        static std::string unescape(const std::string& input);
    };

    // A persistent value that is stored with a unique key specified by an object name and a series of
    // zero or more sub-keys. The sub-keys are used to create a unique storage key for each element within a
    // (possibly nested) collection such as a map or set.
    //
    // For non-collection types, declare an object of type persistent<type> and initialize it with
    // { storage, default-value, unique-key }. If the variable already exists in storage it will be initialized
    // with its stored state. If not it will use the provided default value.
    //
    // For collections, at instantiation time (e.g. insert, emplace, etc), insert the value as
    // { storage, value, unique-object-name, key-name, key-name... }, where the key-names are the
    // keys for each nested map, set, vector, etc.
    // E.g. for map<int, persistent<std::string>> items, you would insert a string mapped to the integer 9 using:
    // map.insert(std::make_pair(9, {storage, "value", "items", 9}); which would yield a storage key "items_00009".
    //
    // In the case of nested maps, multiple subkeys are needed to guarantee the uniqueness of the generated
    // storage key. e.g. for map<int, map<char, persistent<string>>> nesty; you would insert an item in the 7 map,
    // mapped to the char 'x' like so: nesty[7]['x'] = {storage, "value", "nesty", 'x', 7}; which would yield a
    // storage key "nesty_x_00007".
    //
    // For consistency, it's recommended that in nested collections the sub-keys be ordered from inner to outer.
    //
    // To initialize the contents of a collection from persistent storage at startup, use one of the initialize()
    // methods defined below, providing a lambda function which emplaces each provided key(s)/value into the container.
    // E.g. to initialize a map<int, persistent<string>> you would do:
    //   persistent<string>::initialize<int>(storage, "mapname", [&](auto value, auto key){ map.emplace(key, value); });
    //
    // Each type used as a value or a key requires specialized to_string and from_string methods. Implementation are
    // provided for std::string, uint64_t and a couple of types used by pbft. Unfortunately it's not possible to
    // implement a generic *_string methods for tuples, or even for each tuple arity, as C++ doesn't allow
    // specialization for (non-concrete) templated types without specializing the entire class template.
    //
    template <typename T>
    class persistent : public persist_base
    {
    public:
        // construct a persistent variable, retrieving its value from storage if it exists, otherwise
        // initializing it with the provided default.
        template <typename... K>
        persistent(std::shared_ptr<bzn::storage_base> persist_storage, const T &default_value, const std::string& name, K... subkeys)
            : storage(persist_storage), key(escape(name) + generate_key(subkeys...))
        {
            if (this->storage)
            {
                auto val = this->storage->read(STATE_UUID, this->key);
                if (val)
                {
                    t = from_string(val.value());
                }
                else
                {
                    t = default_value;
                    auto result = this->storage->create(STATE_UUID, this->key, to_string(t));
                    if (result != storage_result::ok)
                    {
                        throw std::runtime_error("Error initializing value in storage");
                    }
                }
            }
            else
            {
                throw std::runtime_error("No persistent storage defined");
            }
        }

        // conversion constructor for comparison in maps etc. without placing in storage
        // made explicit to avoid unintentionally assign a raw type to a persistent
        explicit persistent(const T &value)
        : t(value)
        {}

        // default constructor to allow use of [] and insert_or_assign in maps, etc.
        persistent()
        {}

        // assign a new value to a persistent variable. the new value is immediately placed in storage
        persistent<T>& operator=(const T& value)
        {
            t = value;
            if (this->storage)
            {
                this->storage->update(STATE_UUID, this->key, to_string(value));
            }
            else
            {
                throw(std::runtime_error("Object has no storage"));
            }

            return *this;
        }

        // since the lifetime of the value is not tied to the object, it's necessary to explicitly
        // destroy it in order to remove from storage
        void destroy()
        {
            if (this->storage)
            {
                this->storage->remove(STATE_UUID, this->key);
            }
            else
            {
                throw(std::runtime_error("Object has no storage"));
            }
        }

        // get the value of the variable
        const T& value() const
        {
            return t;
        }

        // comparison operator, mainly for ordering in collections
        bool operator<(const persistent<T>& rhs) const
        {
            return t < rhs.t;
        }

        // test for equality
        bool operator==(const persistent<T>& rhs) const
        {
            return t == rhs.t;
        }

        static T from_string(const std::string& /*value*/)
        {
            // this method needs to be specialized for each type used
            assert(0);
            return T{};
        }

        static std::string to_string(const T& /*value*/)
        {
            // this method needs to be specialized for each type used
            assert(0);
            return std::string{};
        }

        // initialize values in a collection
        template<typename A>
        static void
        initialize(std::shared_ptr<bzn::storage_base> storage, const std::string& basename
            , std::function<void(const persistent<T>&, const A&)> store)
        {
            auto escaped_base = escape(basename);
            auto results = storage->get_keys_if(STATE_UUID, escaped_base + std::string{SEPARATOR}
                , escaped_base + std::string{SEPARATOR + 1});
            for (const auto& res : results)
            {
                std::string key_str = unescape(res.substr(escaped_base.size() + 1));
                A key{persistent<A>::from_string(key_str)};

                LOG(debug) << "initializing state " << res << " from storage";

                // instantiate this member from storage and pass it to be emplaced into container
                store(persistent<T>{storage, {}, basename, key}, key);
            }
        }

        // initialize values in a nested collection
        // note - we need a version of this function for each number of sub-keys
        template<typename A, typename B>
        static void
        initialize(std::shared_ptr<bzn::storage_base> storage, const std::string& basename
            , std::function<void(const persistent<T>&, const A&, const B&)> store)
        {
            auto escaped_base = escape(basename);
            auto results = storage->get_keys_if(STATE_UUID, escaped_base + std::string{SEPARATOR}
                , escaped_base + std::string{SEPARATOR + 1});
            for (const auto& res : results)
            {
                std::tuple<A, B> subkeys = extract_subkeys<A, B>(res.substr(escaped_base.size() + 1));

                LOG(debug) << "initializing state " << res << " from storage";

                // instantiate this member from storage and pass it to be emplaced into container
                store(persistent<T>{storage, {}, basename, std::get<0>(subkeys), std::get<1>(subkeys)}
                    , std::get<0>(subkeys), std::get<1>(subkeys));
            }
        }

        // note - this could be expanded into a variadic to support an arbitrary number of sub-keys
        template<typename A, typename B>
        static std::tuple<A, B>
        extract_subkeys(const std::string& key)
        {
            // find unescaped separator
            size_t offset{0};
            while (offset < key.size())
            {
                offset = key.find(SEPARATOR, offset);
                if (offset >= key.size() || key[offset + 1] != SEPARATOR)
                {
                    break;
                }

                offset += 2;
            }

            assert(offset < key.size());
            std::string v0 = key.substr(0, offset);
            std::string v1 = key.substr(offset + 1);
            return {persistent<A>::from_string(unescape(v0)), persistent<B>::from_string(unescape(v1))};
        }

    private:
        T t;
        std::shared_ptr<bzn::storage_base> storage;
        std::string key;

        std::string generate_key()
        {
            return "";
        }

        template <typename K, typename... Rest>
        std::string generate_key(K k, Rest... rest)
        {
            return std::string{SEPARATOR} + escape(persistent<K>::to_string(k)) + generate_key(rest...);
        }
    };
}
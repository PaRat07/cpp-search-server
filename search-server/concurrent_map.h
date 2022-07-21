#pragma once

#include <algorithm>
#include <cstdlib>
#include <future>
#include <map>
#include <numeric>
#include <random>
#include <string>
#include <vector>

using namespace std::literals::string_literals;

template <typename Key, typename Value>
class ConcurrentMap {
public:
    static_assert(std::is_integral_v<Key>, "ConcurrentMap supports only integer keys");
    struct Access {
        Access(const Key& key, std::map<Key,Value>& map1, std::mutex& m)
                : guard(m)
                , ref_to_value(map1[key])
        {

        }
        std::lock_guard<std::mutex> guard;
        Value& ref_to_value;
    };

    explicit ConcurrentMap(size_t bucket_count)
            : buckets_(bucket_count)
    {

    }

    Access operator[](const Key& key) {
        return Access(key, buckets_[key % buckets_.size()].first, buckets_[key % buckets_.size()].second);
    }

    std::map<Key, Value> BuildOrdinaryMap() {
        std::map<Key, Value> ans;
        for (auto& [map, m] : buckets_) {
            std::lock_guard<std::mutex> guard(m);
            for (const auto& [key, value] : map) {
                ans[key] = value;
            }
        }
        return ans;
    }

    std::vector<std::pair<Key, Value>> BuildVecPair() {
        std::vector<std::pair<Key, Value>> ans;
        for (auto& [map, m] : buckets_) {
            std::lock_guard guard(m);
            for (const auto& [key, value] : map) {
                ans.emplace_back(key, value);
            }
        }
        return ans;
    }

    void Erase(const Key& key) {
        std::lock_guard guard(buckets_[key % buckets_.size()].second);
        buckets_[key % buckets_.size()].first.erase(key);
    }
private:
    std::vector<std::pair<std::map<Key, Value>, std::mutex>> buckets_;
};

#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <future>

template <typename Key, typename Value>
class ConcurrentMap {
public:
    static_assert(std::is_integral_v<Key>, "ConcurrentMap supports only integer keys");

    explicit ConcurrentMap(size_t bucket_count) : mutex_maps_(bucket_count) {};
    ConcurrentMap() = default;

    struct Access {
        Access(Value& value, std::mutex& mutex_value)
            :ref_to_value(value), mutex_value_ref(mutex_value) {}
        Value& ref_to_value;
        std::mutex& mutex_value_ref;
        ~Access() {
            mutex_value_ref.unlock();
        }
    };

    Access operator[](const Key& key) {
        uint64_t key_64 = static_cast<uint64_t>(key);
        uint64_t part = key_64 % mutex_maps_.size();
        mutex_maps_[part].mutex_.lock();
        return { mutex_maps_[part].data_[key], mutex_maps_[part].mutex_ };
    }

    std::map<Key, Value> BuildOrdinaryMap() {
        std::map<Key, Value> result;
        for (auto& mm : mutex_maps_) {
            std::lock_guard guard(mm.mutex_);
            result.insert(mm.data_.begin(), mm.data_.end());
        }
        return result;
    }

    struct Mutex_Map {
        std::map<Key, Value> data_ = {};
        std::mutex mutex_;
    };

private:
    std::vector<Mutex_Map> mutex_maps_;
};
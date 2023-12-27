#ifndef __MAP_HPP__
#define __MAP_HPP__

#include <map>
#include <mutex>

template <typename K, typename V, typename Compare = std::less<K> >
class Map {
private:
    std::map<K, V, Compare> map_;
    std::mutex mutex_;

public:
    Map() {}
    ~Map() = default;

    auto insert_or_assign(const K &key, const V &value) {
        // std::lock_guard<std::mutex> lock(mutex_);
        return map_.insert_or_assign(key, value);
    }

    auto erase(const K &key) {
        // std::lock_guard<std::mutex> lock(mutex_);
        return map_.erase(key);
    }

    auto at(const K &key) {
        // std::lock_guard<std::mutex> lock(mutex_);
        return map_.at(key);
    }

    auto find(const K &key) {
        // std::lock_guard<std::mutex> lock(mutex_);
        return map_.find(key);
    }

    auto begin() {
        // std::lock_guard<std::mutex> lock(mutex_);
        return map_.begin();
    }

    auto end() {
        // std::lock_guard<std::mutex> lock(mutex_);
        return map_.end();
    }

    bool clear() {
        // std::lock_guard<std::mutex> lock(mutex_);
        map_.clear();
        return true;
    }

    bool empty() {
        // std::lock_guard<std::mutex> lock(mutex_);
        return map_.empty();
    }
};

#endif

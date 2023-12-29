#ifndef __MAP_HPP__
#define __MAP_HPP__

#include <map>
#include <mutex>

inline void check_lock(std::unique_lock<std::mutex> &lock, std::mutex *mutex2) {
    // Check if lock is locked.
    if (!lock.owns_lock()) {
        throw std::logic_error("Map: lock must be locked");
    }
    // Check if lock is locked by mutex2.
    if (lock.mutex() != mutex2) {
        throw std::logic_error("Map: mutexes must be the same");
    }
}

template <typename K, typename V, typename Compare = std::less<K> >
class Map {
private:
    std::map<K, V, Compare> map_;
    std::mutex mutex_;

public:
    Map() {}
    ~Map() {}

    std::mutex &get_mutex() {
        return mutex_;
    }

    // Read-only operations
    bool empty() {
        std::lock_guard<std::mutex> lock(mutex_);
        return map_.empty();
    }

    // Write operations
    auto insert_or_assign(const K &key, V value, std::unique_lock<std::mutex> &lock) {
        check_lock(lock, &mutex_);
        return map_.insert_or_assign(key, std::move(value));
    }

    auto erase(const K &key, std::unique_lock<std::mutex> &lock) {
        check_lock(lock, &mutex_);
        return map_.erase(key);
    }

    auto erase(typename std::map<K, V, Compare>::iterator it, std::unique_lock<std::mutex> &lock) {
        check_lock(lock, &mutex_);
        return map_.erase(it);
    }

    V &at(const K &key, std::unique_lock<std::mutex> &lock) {
        check_lock(lock, &mutex_);
        return map_.at(key);
    }

    bool clear(std::unique_lock<std::mutex> &lock) {
        check_lock(lock, &mutex_);
        map_.clear();
        return true;
    }

    // Iterator operations
    auto find(const K &key, std::unique_lock<std::mutex> &lock) {
        check_lock(lock, &mutex_);
        return map_.find(key);
    }

    auto begin(std::unique_lock<std::mutex> &lock) {
        check_lock(lock, &mutex_);
        return map_.begin();
    }

    auto end(std::unique_lock<std::mutex> &lock) {
        check_lock(lock, &mutex_);
        return map_.end();
    }

    bool check_exist(const K &key, std::unique_lock<std::mutex> &lock) {
        check_lock(lock, &mutex_);
        return map_.find(key) != map_.end();
    }
};

#endif

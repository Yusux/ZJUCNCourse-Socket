#ifndef __QUEUE_HPP__
#define __QUEUE_HPP__

#include <queue>
#include <mutex>

template <typename T>
class Queue {
private:
    std::queue<T> queue_;
    std::mutex mutex_;

public:
    Queue() {}
    ~Queue() {}

    void push(const T &value) {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push(value);
    }

    T pop() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (queue_.empty()) {
            return T();
        }
        T value = queue_.front();
        queue_.pop();
        return value;
    }

    bool empty() {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }
};

#endif

#include <mutex>
#include <condition_variable>
#include <deque>

#if 0
template <typename T>
class BoundedBlockingQueue {
public:
    explicit BoundedBlockingQueue(int maxSize) : ringbuffer(maxSize) {

    }

    void put(const T& x) {
        std::unique_lock<std::mutex> lock(mutex_);
        notFull_.wait(lock, [&] { return !ringbuffer.full(); });
        ringbuffer.push_back(x);
        if (ringbuffer.size() == 1)
            notEmpty_.notify_one();
    }

    void put(T&& x) {
        std::unique_lock<std::mutex> lock(mutex_);
        notFull_.wait(lock, [&] { return !ringbuffer.full(); });
        ringbuffer.push_back(x);
        if (ringbuffer.size() == 1)
            notEmpty_.notify_one();
    }

    T take() {
        std::unique_lock<std::mutex> lock(mutex_);
        notEmpty_.wait(lock, [&] { return !ringbuffer.empty(); });
        assert(!ringbuffer.empty());
        T front(ringbuffer.front());
        ringbuffer.pop_front();
        notFull_.notify_one();
        return front;
    }

    bool empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return ringbuffer.empty();
    }

    bool full() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return ringbuffer.full();
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return ringbuffer.size();
    }

    size_t capacity() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return ringbuffer.capacity();
    }

private:
    mutable std::mutex  mutex_;
    std::condition_variable notEmpty_;
    std::condition_variable notFull_;
    std::circular_buffer<T> ringbuffer;
};
#endif
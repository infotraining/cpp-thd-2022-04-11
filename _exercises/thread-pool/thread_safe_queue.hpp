#ifndef THREAD_SAFE_QUEUE_HPP
#define THREAD_SAFE_QUEUE_HPP

#include <condition_variable>
#include <mutex>
#include <queue>

template <typename T>
class ThreadSafeQueue
{
public:
    ThreadSafeQueue() = default;

    ThreadSafeQueue(const ThreadSafeQueue&) = delete;
    ThreadSafeQueue& operator=(const ThreadSafeQueue&) = delete;

    bool empty() const
    {
        std::lock_guard<std::mutex> lock(mtx_queue_);
        return queue_.empty();
    }

    void push(const T& item)
    {
        {
            std::lock_guard<std::mutex> lock(mtx_queue_);
            queue_.push(item);
        }
        cv_queue_not_empty_.notify_one();
    }

    void push(T&& item)
    {
        {
            std::lock_guard<std::mutex> lock(mtx_queue_);
            queue_.push(std::move(item));
        }
        cv_queue_not_empty_.notify_one();
    }

    void push(std::initializer_list<T> items)
    {
        {
            std::lock_guard<std::mutex> lock(mtx_queue_);
            for(const auto& item : items)
                queue_.push(std::move(item));
        }
        cv_queue_not_empty_.notify_all();
    }

    bool try_pop(T& item)
    {
        std::unique_lock<std::mutex> lk {mtx_queue_, std::try_to_lock};

        if (lk.owns_lock() && !queue_.empty())
        {
            item = queue_.front();
            queue_.pop();

            return true;
        }

        return false;
    }

    void pop(T& item)
    {
        std::unique_lock<std::mutex> lk {mtx_queue_};

        cv_queue_not_empty_.wait(lk, [this]
            { return !queue_.empty(); });

        item = queue_.front();
        queue_.pop();
    }

private:
    std::queue<T> queue_;
    mutable std::mutex mtx_queue_;
    std::condition_variable cv_queue_not_empty_;
};

#endif // THREAD_SAFE_QUEUE_HPP

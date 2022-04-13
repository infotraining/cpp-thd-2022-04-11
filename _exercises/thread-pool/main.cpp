#include "thread_safe_queue.hpp"

#include <cassert>
#include <chrono>
#include <functional>
#include <future>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

using namespace std::literals;

using Task = std::function<void()>;

namespace ver_1_0
{
    class ThreadPool
    {
    public:
        static inline Task poisoning_pill {};

        explicit ThreadPool(size_t size)
            : threads_(size)
        {
            for (auto& thd : threads_)
            {
                // thd = std::thread{[this]{ run();}};
                thd = std::thread {&ThreadPool::run, this};
            }
        }

        ThreadPool(const ThreadPool&) = delete;
        ThreadPool& operator=(const ThreadPool&) = delete;

        ~ThreadPool()
        {
            for (size_t i = 0; i < threads_.size(); ++i)
                send_poisoning_pill();

            for (auto& thd : threads_)
                if (thd.joinable())
                    thd.join();
        }

        void submit(const Task& task)
        {
            if (is_poisoning_pill(task))
                throw std::invalid_argument("Empty function is not supported");

            q_tasks_.push(task);
        }

    private:
        void send_poisoning_pill()
        {
            q_tasks_.push(poisoning_pill);
        }

        bool is_poisoning_pill(const Task& task) const
        {
            return task == nullptr;
        }

        void run()
        {
            Task task;
            while (true)
            {
                q_tasks_.pop(task);
                if (is_poisoning_pill(task))
                    return;
                task();
            };
        }

        ThreadSafeQueue<Task> q_tasks_;
        std::vector<std::thread> threads_;
    };
}

namespace ver_1_1
{
    class ThreadPool
    {
    public:
        explicit ThreadPool(size_t size)
            : threads_(size)
        {
            for (auto& thd : threads_)
            {
                // thd = std::thread{[this]{ run();}};
                thd = std::thread {&ThreadPool::run, this};
            }
        }

        ThreadPool(const ThreadPool&) = delete;
        ThreadPool& operator=(const ThreadPool&) = delete;

        ~ThreadPool()
        {
            for (size_t i = 0; i < threads_.size(); ++i)
                submit([this] { end_work_ = true; });

            for (auto& thd : threads_)
                if (thd.joinable())
                    thd.join();
        }

        void submit(const Task& task)
        {
            q_tasks_.push(task);
        }

    private:
        void run()
        {
            Task task;
            while (true)
            {
                q_tasks_.pop(task);
                
                task();
                
                if (end_work_)
                    return;
            };
        }

        ThreadSafeQueue<Task> q_tasks_;
        std::vector<std::thread> threads_;
        std::atomic<bool> end_work_{false};
    };
}

void background_work(size_t id, const std::string& text, std::chrono::milliseconds delay)
{
    std::cout << "bw#" << id << " has started in the thread#" << std::this_thread::get_id() << std::endl;

    for (const auto& c : text)
    {
        std::cout << "bw#" << id << ": " << c << " - thread#" << std::this_thread::get_id() << std::endl;

        std::this_thread::sleep_for(delay);
    }

    std::cout << "bw#" << id << " is finished..." << std::endl;
}

int main()
{
    std::cout << "Main thread starts..." << std::endl;
    const std::string text = "Hello Threads";

    ver_1_1::ThreadPool thread_pool(6);

    thread_pool.submit([&]
        { background_work(1, text, 20ms); });
    thread_pool.submit([&]
        { background_work(2, "thread pool", 30ms); });
    thread_pool.submit([&]
        { background_work(3, "thread pool", 30ms); });

    for (size_t i = 4; i <= 40; ++i)
        thread_pool.submit([i]()
            { background_work(i, "thread pool", 30ms); });

    std::cout << "Main thread ends..." << std::endl;
}

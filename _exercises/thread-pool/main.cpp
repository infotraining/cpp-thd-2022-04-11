#include "thread_safe_queue.hpp"

#include <cassert>
#include <chrono>
#include <functional>
#include <future>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <random>

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

        template <typename Callable>
        auto submit(Callable&& callable) -> std::future<decltype(callable())>
        {
            using ResultT = decltype(callable());

            auto pt = std::make_shared<std::packaged_task<ResultT()>>(std::forward<Callable>(callable));
            std::future<ResultT> fresult = pt->get_future();
            
            q_tasks_.push([pt] { (*pt)(); });

            return fresult;
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

int calculate_square(int x)
{
    std::cout << "Starting calculation for " << x << " in " << std::this_thread::get_id() << std::endl;

    std::random_device rd;
    std::uniform_int_distribution<> distr(100, 5000);

    std::this_thread::sleep_for(std::chrono::milliseconds(distr(rd)));

    if (x % 3 == 0)
        throw std::runtime_error("Error#3");

    return x * x;
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


    std::future<int> fs19 = thread_pool.submit([] { return calculate_square(19); });
    std::future<int> fs31 = thread_pool.submit([] { return calculate_square(31); });


    std::cout << "19 * 19 = " << fs19.get() << std::endl;
    std::cout << "31 * 31 = " << fs31.get() << std::endl;

    std::cout << "Main thread ends..." << std::endl;
}

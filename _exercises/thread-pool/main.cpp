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

    ThreadPool thread_pool(6);

    thread_pool.submit([&]
        { background_work(1, text, 20ms); });
    thread_pool.submit([&]
        { background_work(2, "thread pool", 30ms); });
    thread_pool.submit([&]
        { background_work(3, "thread pool", 30ms); });

    std::cout << "Main thread ends..." << std::endl;
}

#include "joining_thread.hpp"

#include <cassert>
#include <chrono>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <mutex>

using namespace std::literals;

template <typename T>
struct Synchronized
{
    T value;
    std::mutex mtx_value;
};

template <typename F_, typename Value_>
void apply(F_ f, Synchronized<Value_>& sync_value)
{
    std::lock_guard lk{sync_value.mtx_value}; // CS starts
    f(sync_value.value);
} // CS ends

void run(uint64_t count, Synchronized<uint64_t>& counter)
{
    for (uint64_t i = 0; i < count; ++i) {
        apply([&](uint64_t& v) { ++v; }, counter);
        
        // counter.mtx_value.lock(); // CS starts
        // ++counter.value;
        // counter.mtx_value.unlock(); // CS ends

        // std::lock_guard lk{counter.mtx_value};
        // ++counter.value;
    }      
}

int main()
{
    std::cout << "Main thread starts..." << std::endl;
    const std::string text = "Hello Threads";

    const uint64_t N = 1'000'000;

    Synchronized<uint64_t> counter{};
 
    {
        ext::JoiningThread thd1 {&run, N, std::ref(counter)};
        ext::JoiningThread thd2 {&run, N, std::ref(counter)};
    } // implicit join

    std::cout << "counter:" << counter.value << "\n";

    std::cout << "Main thread ends..." << std::endl;
}

#include <cassert>
#include <chrono>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

using namespace std::literals;

template <typename T>
struct ThreadResult
{
    T value;
    std::exception_ptr eptr;
};

void background_work(size_t id, const std::string& text, std::chrono::milliseconds delay, ThreadResult<char>& letter)
{
    for (const auto c : text)
        std::cout << "bw#" << id << ": " << c << std::endl;

    try
    {
        letter.value = text.at(3);
        std::cout << "letter: " << letter.value << std::endl;
    }
    catch (...)
    {
        letter.eptr = std::current_exception();
    }
}

int main()
{
    const auto no_of_cores = std::max(1u, std::thread::hardware_concurrency());
    std::cout << "No of hardware threads: " << no_of_cores << "\n";

    std::cout << "Main thread starts..." << std::endl;
    const std::string text = "Hello Threads";

    std::vector<ThreadResult<char>> results(no_of_cores);
    std::vector<std::thread> thds(no_of_cores);

    for (size_t i = 0; i < no_of_cores; ++i)
    {
        thds[i] = std::thread {&background_work, i, "bw_"s + std::to_string(i), 10ms, std::ref(results[i])};
    }

    for (auto& thd : thds)
        if (thd.joinable())
            thd.join();

    std::cout << "\n-----------------------\n";

    for (auto& r : results)
    {
        try
        {
            if (r.eptr)
            {
                std::rethrow_exception(r.eptr);
            }

            std::cout << "Letter: " << r.value << std::endl;
        }
        catch (const std::out_of_range& e)
        {
            std::cout << "CAUGHT out_of_range: " << e.what() << std::endl;
        }
        catch (const std::exception& e)
        {
            std::cout << "CAUGHT exception: " << e.what() << std::endl;
        }
    }

    std::cout << "Main thread ends..." << std::endl;

    ThreadResult<char> rslt;
    std::thread thd{&background_work, 1, "hello", 10ms, std::ref(rslt)};
    std::cout << "id: " << thd.get_id() << std::endl;
    thd.join();

    thd = std::thread{&background_work, 1, "hello", 10ms, std::ref(rslt)};
    std::cout << "id: " << thd.get_id() << std::endl;
    thd.join();
}

#include <cassert>
#include <chrono>
#include <functional>
#include <future>
#include <iostream>
#include <random>
#include <string>
#include <thread>
#include <tuple>
#include <vector>

using namespace std::literals;

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

void save_to_file(const std::string& filename)
{
    std::cout << "Saving to file: " << filename << std::endl;

    std::this_thread::sleep_for(3s);

    std::cout << "File saved: " << filename << std::endl;
}

void wtf()
{
    auto f1 = std::async(std::launch::async, &save_to_file, "data1.txt");
    auto f2 = std::async(std::launch::async, &save_to_file, "data2.txt");
    auto f3 = std::async(std::launch::async, &save_to_file, "data3.txt");
}

template <typename Callable>
auto launch_async(Callable&& callable)
{
    using ResultT = decltype(callable());
    std::packaged_task<ResultT()> pt{std::forward<Callable>(callable)};
    std::future<ResultT> f = pt.get_future();
    
    std::thread thd{std::move(pt)};
    thd.detach();

    return f;
}

void no_wtf()
{
    launch_async([] { save_to_file("data1.txt"); });
    launch_async([] { save_to_file("data2.txt"); });
    launch_async([] { save_to_file("data3.txt"); });
}

class SquareCalculator
{
    std::promise<int> promise_;
public:
    void calculate(int x)
    {
        try
        {
            int result = calculate_square(x);
            promise_.set_value(result);
        }
        catch(const std::exception&)
        {
            promise_.set_exception(std::current_exception());
        }        
    }

    std::future<int> get_future()
    {
        return promise_.get_future();
    }
};

void using_promise()
{
    SquareCalculator calc; // asynchronous provider
    std::future<int> f = calc.get_future();
    std::shared_future<int> shared_f1 = f.share();

    std::thread thd_consumer1{[shared_f1] { 
        auto result = shared_f1.get();
        std::cout << "Consuming: " << result << " in thd " << std::this_thread::get_id() << std::endl;
    }};

    std::thread thd_consumer2{[shared_f1] { 
        auto result = shared_f1.get();
        std::cout << "Consuming: " << result << " in thd " << std::this_thread::get_id() << std::endl;
    }};

    calc.calculate(23);

    thd_consumer1.join();
    thd_consumer2.join();
}

int main()
{
    // {
    //     std::cout << "Main thread id: " << std::this_thread::get_id() << std::endl;
    //     std::future<int> f1 = std::async(std::launch::async, &calculate_square, 13);
    //     std::future<int> f2 = std::async(std::launch::deferred, &calculate_square, 9);
    //     std::future<void> async_save_f = std::async(std::launch::async, &save_to_file, "data.txt");

    //     while (async_save_f.wait_for(100ms) != std::future_status::ready)
    //     {
    //         std::cout << "I'm still waiting..." << std::endl;
    //     }

    //     std::cout << "result of f1: " << f1.get() << std::endl;
    //     try
    //     {
    //         auto result_of_f2 = f2.get();
    //         std::cout << "result of f2: " << result_of_f2 << std::endl;
    //     }
    //     catch (const std::exception& e)
    //     {
    //         std::cout << "CAUGHT: " << e.what() << std::endl;
    //     }

    //     std::vector<int> args = {1, 2, 4, 5, 7, 8};
    //     std::vector<std::tuple<int, std::future<int>>> results;

    //     for (const auto& arg : args)
    //     {
    //         results.push_back(std::tuple(arg, std::async(std::launch::async, &calculate_square, arg)));
    //     }

    //     for (auto& [arg, fsquare] : results)
    //     {
    //         std::cout << arg << " - " << fsquare.get() << std::endl;
    //     }

    //     std::cout << "\n------------------------------------\n";

    //     // wtf();
    // }

    // // packaged_task
    // {
    //     std::packaged_task<int()> pt1 {[]
    //         { return calculate_square(19); }};

    //     std::future<int> f1 = pt1.get_future();

    //     std::thread thd1{std::move(pt1)};
    //     thd1.detach();

    //     std::cout << "19 * 19 = " << f1.get() << std::endl;
    // }

    // no_wtf();

    // std::this_thread::sleep_for(2s);

    using_promise();
}

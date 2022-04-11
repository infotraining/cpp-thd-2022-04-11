#include <cassert>
#include <chrono>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include "joining_thread.hpp"

using namespace std::literals;

void simple_task()
{
    for (int i = 0; i < 10; ++i)
    {
        std::cout << "simple_task: " << i << std::endl;
        // std::this_thread::sleep_for(std::chrono::milliseconds(250));
        std::this_thread::sleep_for(250ms);
    }
}

void background_work(size_t id, const std::string& text, std::chrono::milliseconds delay)
{
    std::cout << "bw#" << id << " has started..." << std::endl;

    for (const auto& c : text)
    {
        std::cout << "bw#" << id << ": " << c << std::endl;

        std::this_thread::sleep_for(delay);
    }

    std::cout << "bw#" << id << " is finished..." << std::endl;
}

class BackgroundWork
{
    const int id_;
    const std::string text_;

public:
    BackgroundWork(int id, std::string text)
        : id_ {id}
        , text_ {std::move(text)}
    {
    }

    void operator()(std::chrono::milliseconds delay)
    {
        std::cout << "BW#" << id_ << " has started..." << std::endl;

        for (const auto& c : text_)
        {
            std::cout << "BW#" << id_ << ": " << c << std::endl;

            std::this_thread::sleep_for(delay);
        }

        std::cout << "BW#" << id_ << " is finished..." << std::endl;
    }
};

std::thread create_thread(int id)
{
    return std::thread {&background_work, id, "text", 150ms};
}

int main()
{
    std::cout << "Main thread starts..." << std::endl;
    const std::string text = "Hello Threads";

    std::thread thd_empty;

    std::thread thd_1 {&simple_task};
    assert(thd_1.joinable());
    // std::thread thd_2{&background_work, 1, std::cref(text), 5s};
    std::thread thd_2 {[&text]()
        { background_work(1, text, 5s); }};
    std::thread thd_3 {&background_work, 2, "Thread", 200ms};
    BackgroundWork bw {3, "!!!"};
    std::thread thd_4 {std::ref(bw), 400ms};
    std::thread thd_5 {[text]
        { background_work(4, text, 50ms); }};

    std::vector<std::thread> thds(5);
    thds[0] = std::move(thd_1);
    thds[1] = std::move(thd_2);
    thds[2] = std::move(thd_3);
    thds[3] = std::move(thd_4);
    thds[4] = std::move(thd_5);
    thds.push_back(create_th+*read(5));

    std::cout << "I'm going to sleep" << std::endl;

    thds[1].detach();
    assert(!thds[1].joinable());

    std::this_thread::sleep_for(10s);

    std::cout << "After sleep" << std::endl;

    for (auto& thd : thds)
        if (thd.joinable())
            thd.join();

    std::cout << "thd_empty.id: " << thd_empty.get_id() << std::endl;

    ext::JoiningThread thd_reusable {&simple_task};
    thd_reusable.join();
    thd_reusable = ext::JoiningThread {&background_work, 6, "Reusable", 50ms};

    //ext::JoiningThread thd_copied(thd_reusable);

    std::cout << "Main thread ends..." << std::endl;
}

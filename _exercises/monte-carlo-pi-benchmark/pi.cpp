#define CATCH_CONFIG_ENABLE_BENCHMARKING
#define CATCH_CONFIG_MAIN

#include "catch.hpp"

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <random>
#include <thread>

using namespace std;

namespace ext
{
    constexpr auto hardware_destructive_interference_size = 64U;
}

namespace SingleThread
{
    double calculatePi(uint64_t totalTrials)
    {
        std::random_device rd;
        mt19937_64 rnd_gen {rd()};
        uniform_real_distribution<> distr(0, 1);

        uint64_t totalHits {};

        for (uint64_t n = 0; n < totalTrials; ++n)
        {
            double x = distr(rnd_gen);
            double y = distr(rnd_gen);
            if (x * x + y * y < 1)
            {
                totalHits++;
            }
        }

        const double pi = static_cast<double>(totalHits) / totalTrials * 4;

        return pi;
    }
}

namespace Multithreading
{
    namespace ver1
    {

        void calculateHits(size_t seed, uint64_t noOfTrials, uint64_t& result)
        {
            mt19937_64 rnd_gen {seed};
            uniform_real_distribution<> distr(0, 1);

            for (uint64_t n = 0; n < noOfTrials; ++n)
            {
                double x = distr(rnd_gen);
                double y = distr(rnd_gen);
                if (x * x + y * y < 1)
                {
                    result++;
                }
            }
        }

        double calculatePi(uint64_t totalTrials, uint16_t countOfThreads)
        {
            auto trialsPerWorker = totalTrials / countOfThreads;

            vector<std::thread> workers(countOfThreads);
            vector<uint64_t> results(countOfThreads);

            std::random_device rd;
            for (auto i = 0; i < countOfThreads; ++i)
            {
                size_t seed = rd();
                workers[i] = thread {[&results, seed, trialsPerWorker, i]
                    { calculateHits(seed, trialsPerWorker, results[i]); }};
            }

            for (auto& workerThread : workers)
                workerThread.join();

            auto totalHits = std::accumulate(results.begin(), results.end(), 0.0);

            const double pi = static_cast<double>(totalHits) / totalTrials * 4;

            return pi;
        }
    }

    namespace ver2
    {

        void calculateHits(size_t seed, uint64_t noOfTrials, uint64_t& result)
        {
            mt19937_64 rnd_gen {seed};
            uniform_real_distribution<> distr(0, 1);

            uint64_t local_counter {};

            for (uint64_t n = 0; n < noOfTrials; ++n)
            {
                double x = distr(rnd_gen);
                double y = distr(rnd_gen);
                if (x * x + y * y < 1)
                {
                    local_counter++;
                }
            }

            result += local_counter;
        }

        double calculatePi(uint64_t totalTrials, uint16_t countOfThreads)
        {
            auto trialsPerWorker = totalTrials / countOfThreads;

            vector<std::thread> workers(countOfThreads);
            vector<uint64_t> results(countOfThreads);

            std::random_device rd;
            for (auto i = 0; i < countOfThreads; ++i)
            {
                size_t seed = rd();
                workers[i] = thread {[&results, seed, trialsPerWorker, i]
                    { calculateHits(seed, trialsPerWorker, results[i]); }};
            }

            for (auto& workerThread : workers)
                workerThread.join();

            auto totalHits = std::accumulate(results.begin(), results.end(), 0.0);

            const double pi = static_cast<double>(totalHits) / totalTrials * 4;

            return pi;
        }
    }

    namespace ver3
    {
        struct AlignedCounter
        {
            alignas(ext::hardware_destructive_interference_size) uint64_t value = 0;
        };

        void calculateHits(size_t seed, uint64_t noOfTrials, AlignedCounter& result)
        {
            mt19937_64 rnd_gen {seed};
            uniform_real_distribution<> distr(0, 1);

            for (uint64_t n = 0; n < noOfTrials; ++n)
            {
                double x = distr(rnd_gen);
                double y = distr(rnd_gen);
                if (x * x + y * y < 1)
                {
                    result.value++;
                }
            }
        }

        double calculatePi(uint64_t totalTrials, uint16_t countOfThreads)
        {
            auto trialsPerWorker = totalTrials / countOfThreads;

            vector<std::thread> workers(countOfThreads);
            vector<AlignedCounter> results(countOfThreads);

            std::random_device rd;
            for (auto i = 0; i < countOfThreads; ++i)
            {
                size_t seed = rd();
                workers[i] = thread {[&results, seed, trialsPerWorker, i]
                    { calculateHits(seed, trialsPerWorker, results[i]); }};
            }

            for (auto& workerThread : workers)
                workerThread.join();

            auto totalHits = std::accumulate(results.begin(), results.end(), 0ull, [](uint64_t total, AlignedCounter c)
                { return total + c.value; });

            const double pi = static_cast<double>(totalHits) / totalTrials * 4;

            return pi;
        }
    }

    namespace ver4
    {
        uint64_t calculateHits(size_t seed, uint64_t noOfTrials)
        {
            mt19937_64 rnd_gen {seed};
            uniform_real_distribution<> distr(0, 1);
            uint64_t localCounter{};
            for (uint64_t n = 0; n < noOfTrials; ++n)
            {
                double x = distr(rnd_gen);
                double y = distr(rnd_gen);
                if (x * x + y * y < 1)
                {
                    ++localCounter;
                }
            }
            return localCounter;
        }

        double calculatePi(uint64_t totalTrials, uint16_t countOfThreads)
        {
            auto trialsPerWorker = totalTrials / countOfThreads;

            vector<std::future<uint64_t>> futures(countOfThreads);

            std::random_device rd;
            for (auto& f_task : futures)
            {
                // size_t seed = rd();
                f_task = std::async(std::launch::async, &calculateHits, rd(), trialsPerWorker);
            }

            uint64_t totalHits{};
            for (auto& f_task : futures)
                totalHits += f_task.get();

            const double pi = static_cast<double>(totalHits) / totalTrials * 4;

            return pi;
        }
    }
}

template <typename T>
struct Synchronized
{
    T value;
    std::mutex mtx_value;
};

template <typename F_, typename Value_>
void apply(F_ f, Synchronized<Value_>& sync_value)
{
    std::lock_guard lk {sync_value.mtx_value};
    f(sync_value.value);
}

namespace Multithreading
{
    namespace SharedMutableState
    {
        namespace WithMutex
        {
            void calculateHits(size_t seed, uint64_t noOfTrials, Synchronized<uint64_t>& hits)
            {
                mt19937_64 rnd_gen {seed};
                uniform_real_distribution<> distr(0, 1);

                for (uint64_t n = 0; n < noOfTrials; ++n)
                {
                    double x = distr(rnd_gen);
                    double y = distr(rnd_gen);
                    if (x * x + y * y < 1)
                    {
                        apply([](uint64_t& value) { ++value; }, hits);
                    }
                }
            }

            double calculatePi(uint64_t totalTrials, uint16_t countOfThreads)
            {
                auto trialsPerWorker = totalTrials / countOfThreads;

                vector<std::thread> workers(countOfThreads);
                Synchronized<uint64_t> totalHits {};

                std::random_device rd;
                for (auto i = 0; i < countOfThreads; ++i)
                {
                    size_t seed = rd();
                    workers[i] = thread {[&totalHits, seed, trialsPerWorker]
                        { calculateHits(seed, trialsPerWorker, totalHits); }};
                }

                for (auto& workerThread : workers)
                    workerThread.join();

                const double pi = static_cast<double>(totalHits.value) / totalTrials * 4;

                return pi;
            }
        }

        namespace WithAtomic
        {
            void calculateHits(size_t seed, uint64_t noOfTrials, std::atomic<uint64_t>& hits)
            {
                mt19937_64 rnd_gen {seed};
                uniform_real_distribution<> distr(0, 1);

                for (uint64_t n = 0; n < noOfTrials; ++n)
                {
                    double x = distr(rnd_gen);
                    double y = distr(rnd_gen);
                    if (x * x + y * y < 1)
                    {
                        //++hits;
                        hits.fetch_add(1ull, std::memory_order_relaxed);
                    }
                }
            }

            double calculatePi(uint64_t totalTrials, uint16_t countOfThreads)
            {
                auto trialsPerWorker = totalTrials / countOfThreads;

                vector<std::thread> workers(countOfThreads);
                std::atomic<uint64_t> totalHits {};

                std::random_device rd;
                for (auto i = 0; i < countOfThreads; ++i)
                {
                    size_t seed = rd();
                    workers[i] = thread {[&totalHits, seed, trialsPerWorker]
                        { calculateHits(seed, trialsPerWorker, totalHits); }};
                }

                for (auto& workerThread : workers)
                    workerThread.join();

                const double pi = static_cast<double>(totalHits) / totalTrials * 4;

                return pi;
            }
        }
    }
}

TEST_CASE("MonteCarlo Pi")
{
    const uint64_t N = 4'000'000;
    auto countOfThreads = max(1u, thread::hardware_concurrency());

    BENCHMARK("SingleThread")
    {
        const auto pi = SingleThread::calculatePi(N);
        return pi;
    };

    BENCHMARK("MultiThread - hot update")
    {
        return Multithreading::ver1::calculatePi(N, countOfThreads);
    };

    BENCHMARK("MultiThread - local counter")
    {
        return Multithreading::ver2::calculatePi(N, countOfThreads);
    };

    BENCHMARK("MultiThread - padding")
    {
        return Multithreading::ver3::calculatePi(N, countOfThreads);
    };

    BENCHMARK("MultiThread - futures")
    {
        return Multithreading::ver4::calculatePi(N, countOfThreads);
    };

    BENCHMARK("MultiThread - mutex")
    {
        return Multithreading::SharedMutableState::WithMutex::calculatePi(N, countOfThreads);
    };

    BENCHMARK("MultiThread - atomics")
    {
        return Multithreading::SharedMutableState::WithAtomic::calculatePi(N, countOfThreads);
    };
}

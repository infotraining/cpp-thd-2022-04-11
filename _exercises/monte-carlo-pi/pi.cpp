#include <atomic>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <random>
#include <thread>

using namespace std;

void calculatePi(size_t seed, uint64_t noOfTrials, uint64_t& result)
{
    mt19937_64 rnd_gen{seed};
    uniform_real_distribution<> distr(0, 1);

    uint64_t local_counter{};

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

void calculatePiMultithreading(unsigned long totalTrials, unsigned int countOfThreads)
{
    cout << "Multi threaded version" << endl;
    cout << "Pi calculation started! Number of threads:" << countOfThreads << endl;
    
    const auto start = chrono::high_resolution_clock::now();
    
    auto trialsPerWorker = totalTrials / countOfThreads;
    
    vector<std::thread> workers(countOfThreads);
    vector<uint64_t> results(countOfThreads);
       
    std::random_device rd;
    for (auto i = 0; i < countOfThreads; ++i)
    {
        size_t seed = rd();
        //workers[i] = thread{&calculatePi, seed,trialsPerWorker, std::ref(results[i])};
        workers[i] = thread{[&results, seed, trialsPerWorker, i] { calculatePi(seed, trialsPerWorker, results[i]); } };
    }
    
    for (auto& workerThread : workers)
    {
        workerThread.join();
    }

    long totalHits = 0;
    for (auto& partialRes : results)
    {
        totalHits += partialRes;
    }
    const double pi = static_cast<double>(totalHits) / totalTrials * 4;

    const auto end = chrono::high_resolution_clock::now();
    const auto elapsed_time = chrono::duration_cast<chrono::milliseconds>(end - start).count();

    cout << "Pi = " << pi << endl;
    cout << "Elapsed = " << elapsed_time << "ms" << endl;
}

int main()
{
    const long N = 100'000'000;

    // Multi-threaded:
    auto countOfThreads = max(1u, thread::hardware_concurrency());
    calculatePiMultithreading(N, countOfThreads);
    calculatePiMultithreading(N, countOfThreads / 2);
    calculatePiMultithreading(N, countOfThreads / 4);
    calculatePiMultithreading(N, 1);
}

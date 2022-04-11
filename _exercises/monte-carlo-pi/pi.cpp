#include <atomic>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <random>
#include <thread>

using namespace std;

void calculatePi(long noOfTrials, long& result)
{
    result = 0;
    for (long n = 0; n < noOfTrials; ++n)
    {
        double x = rand() / static_cast<double>(RAND_MAX);
        double y = rand() / static_cast<double>(RAND_MAX);
        if (x * x + y * y < 1)
            result++;
    }
}

void calculatePiMultithreading(unsigned long totalTrials, unsigned int countOfThreads)
{
    cout << "Multi threaded version" << endl;
    cout << "Pi calculation started! Number of threads:" << countOfThreads << endl;
    
    const auto start = chrono::high_resolution_clock::now();
    
    auto trialsPerWorker = totalTrials / countOfThreads;
    
    vector<std::thread> workers(countOfThreads);
    vector<long> results(countOfThreads);
       
    for (auto i = 0; i < countOfThreads; ++i)
    {
        workers[i] = thread {calculatePi, trialsPerWorker, std::ref(results[i])};
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

    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    {
        //////////////////////////////////////////////////////////////////////////////
        // single thread
        cout << "Single threaded version" << endl;
        cout << "Pi calculation started!" << endl;
        const auto start = chrono::high_resolution_clock::now();

        long hits = 0;

        for (long n = 0; n < N; ++n)
        {
            double x = rand() / static_cast<double>(RAND_MAX);
            double y = rand() / static_cast<double>(RAND_MAX);
            if (x * x + y * y < 1)
                hits++;
        }

        const double pi = static_cast<double>(hits) / N * 4;

        const auto end = chrono::high_resolution_clock::now();
        const auto elapsed_time = chrono::duration_cast<chrono::milliseconds>(end - start).count();

        cout << "Pi = " << pi << endl;
        cout << "Elapsed = " << elapsed_time << "ms" << endl;

        //////////////////////////////////////////////////////////////////////////////
    }

    // Multi-threaded:
    auto countOfThreads = max(1u, thread::hardware_concurrency());
    calculatePiMultithreading(N, countOfThreads);
    calculatePiMultithreading(N, countOfThreads / 2);
    calculatePiMultithreading(N, countOfThreads / 4);
    calculatePiMultithreading(N, 1);

    // {
    //     cout << "Multi threaded version" << endl;
    //     cout << "Pi calculation started!" << endl;
    //     const auto start = chrono::high_resolution_clock::now();

    //     // auto countOfThreads = max(1u, thread::hardware_concurrency());
    //     // auto countOfThreads = max(1u, thread::hardware_concurrency() / 2);
    //     auto countOfThreads = 2;
    //     auto trialsPerWorker = N/countOfThreads;
    //     vector<std::thread> workers(countOfThreads);
    //     vector<long> results(countOfThreads);
    //     cout << "Count of threads:" << countOfThreads << endl;
    //     for(auto i=0; i< countOfThreads; ++i) {
    //         workers[i] = thread{calculatePi, trialsPerWorker, std::ref(results[i])};
    //     }
    //     for(auto& workerThread : workers) {
    //         workerThread.join();
    //     }

    //     long totalHits = 0;
    //     for(auto& partialRes : results) {
    //         totalHits += partialRes;
    //     }
    //    const double pi = static_cast<double>(totalHits) / N * 4;

    //     const auto end = chrono::high_resolution_clock::now();
    //     const auto elapsed_time = chrono::duration_cast<chrono::milliseconds>(end - start).count();

    //     cout << "Pi = " << pi << endl;
    //     cout << "Elapsed = " << elapsed_time << "ms" << endl;
    // }
}

#include <iostream>
#include <thread>
#include <mutex>

class BankAccount
{
    const int id_;
    double balance_;
    mutable std::recursive_mutex balance_mtx;

public:
    BankAccount(int id, double balance)
        : id_(id)
        , balance_(balance)
    {
    }

    void print() const
    {
        std::cout << "Bank Account #" << id_ << "; Balance = " << balance() << std::endl;
    }

    void transfer(BankAccount& to, double amount)
    {
        std::unique_lock<std::recursive_mutex> lk_from{balance_mtx, std::defer_lock};                                                                                   
        std::unique_lock<std::recursive_mutex> lk_to{to.balance_mtx, std::defer_lock}; 
        std::lock(lk_from, lk_to); // deadlock protection  - CS starts

        balance_ -= amount;
        to.balance_ += amount;
    } // CS ends

    void transfer_alternative_take(BankAccount& to, double amount)
    {
        std::lock(balance_mtx, to.balance_mtx); // deadlock protection  - CS starts
        std::lock_guard lk_from{balance_mtx, std::adopt_lock}; // RAII only                                                                                  
        std::lock_guard lk_to{to.balance_mtx, std::adopt_lock}; 

        balance_ -= amount;
        to.balance_ += amount;
    } // CS ends

    void transfer_alternative_take_cpp17(BankAccount& to, double amount)
    {
        std::scoped_lock lk{balance_mtx, to.balance_mtx };

        balance_ -= amount;
        to.balance_ += amount;
    } // CS ends

    void withdraw(double amount)
    {
        std::lock_guard lk(balance_mtx);
        balance_ -= amount;
    }

    void deposit(double amount)
    {
        std::lock_guard lk(balance_mtx);
        balance_ += amount;
    }

    int id() const
    {
        return id_;
    }

    double balance() const
    {
        std::lock_guard lk(balance_mtx);
        return balance_;
    }

    void lock()
    {
        balance_mtx.lock();
    }

    void unlock()
    {
        balance_mtx.unlock();
    }

    auto with_lock() const
    {
        return std::unique_lock{balance_mtx}; // lock
    }
};

void make_withdraws(BankAccount& ba, int no_of_operations)
{
    for (int i = 0; i < no_of_operations; ++i)
        ba.withdraw(1.0);
}

void make_deposits(BankAccount& ba, int no_of_operations)
{
    for (int i = 0; i < no_of_operations; ++i)
        ba.deposit(1.0);
}

void make_transfers(BankAccount& from, BankAccount& to, int no_of_operations)
{
    for (int i = 0; i < no_of_operations; ++i)
        from.transfer(to, 1.0);
}

int main()
{
    const int NO_OF_ITERS = 10'000'000;

    BankAccount ba1(1, 10'000);
    BankAccount ba2(2, 10'000);

    std::cout << "Before threads are started: ";
    ba1.print();
    ba2.print();

    std::thread thd1(&make_withdraws, std::ref(ba1), NO_OF_ITERS);
    std::thread thd2(&make_deposits, std::ref(ba1), NO_OF_ITERS);
    std::thread thd3(&make_transfers, std::ref(ba1), std::ref(ba2), NO_OF_ITERS);
    std::thread thd4(&make_transfers, std::ref(ba2), std::ref(ba1), NO_OF_ITERS);

    { // transaction
        std::lock_guard lk_transaction{ba1}; // CS starts
        ba1.deposit(1'000'000);
        ba1.transfer(ba2, 500'000);
        ba1.withdraw(500'000);
    } // CS ends

    { 
        auto lk = ba1.with_lock(); // CS starts
        ba1.deposit(1'000'000);
        ba1.transfer(ba2, 500'000);
        ba1.withdraw(500'000);
    } // CS ends

    thd1.join();
    thd2.join();   
    thd3.join();
    thd4.join();

    std::cout << "After all threads are done: ";
    ba1.print();
    ba2.print();
}

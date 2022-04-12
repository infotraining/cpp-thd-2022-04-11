#include "thread_safe_lookup_table.hpp"
#include <iostream>
#include <string>

using namespace std;

int main()
{
    cout << "lookup-table" << endl;

    ThreadSafeLookupTable<int, std::string> tslt;
    string str = "txt";
}

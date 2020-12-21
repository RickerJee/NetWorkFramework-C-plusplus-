// TestTCMalloc.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <chrono>
#include <memory>

template< typename T >
struct array_deleter
{
    void operator ()(const T * p)
    {
        delete[] p;
    }
};

int main()
{
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 100;++i)
    {
        int* a = new int[128];

        delete[] a;

        float* b = new float();

        delete b;

        int* c = new int[1024 * 10];

        delete[]c;

        int* d = new int[1024 * 1024 * 3];

        delete[] d;
    }


    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);


    std::cout << "Timer: "<<duration.count()/1000.0f <<" ms\n";

    std::unique_ptr<int[]> uni_ptr(new int[10]);
    std::shared_ptr<int> sha_ptr(new int[10], [](int* p) {delete[]p; });
    std::shared_ptr<int> sha_ptr2(new int[10], array_deleter<int>());

    getchar();
}

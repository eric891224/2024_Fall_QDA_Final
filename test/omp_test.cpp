#include <omp.h>
#include <iostream>

int main()
{
#pragma omp parallel for num_threads(48)
    for (int i = 0; i < 1000000; i++)
    {
        std::cout << i << std::endl;
    }

    return 0;
}
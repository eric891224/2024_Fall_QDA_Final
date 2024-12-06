#include <fcntl.h>
#include <unistd.h>
#include <chrono>
#include "state.hpp"

int main()
{
    int n = 30;
    int chunk_size = 67108864;
    long long int numAmpTotal = 1<<n;

    Complex *amp = (Complex *)calloc(chunk_size, sizeof(Complex));
    int fd = open("../rx.mem", O_RDWR);
    
    

    // read
   long long total_time = 0;
    for (int i = 0; i < numAmpTotal; i+=chunk_size)
    {   
        std::chrono::steady_clock::time_point t1 = std::chrono::steady_clock::now();
        pread(fd, amp, chunk_size*sizeof(Complex), i * sizeof(Complex));
        std::chrono::steady_clock::time_point t2 = std::chrono::steady_clock::now();

        total_time += std::chrono::duration_cast<std::chrono::microseconds>(t2-t1).count();
        //printf("Statevec real: %f, imag: %f\n", amp->real, amp->imag);
    }

    double throughput = (double)(numAmpTotal*16) / (double)total_time / 1000;
    printf("total time = %lld us\n", total_time);
    printf("read throughput = %f GB/s\n", throughput);
    /*
    // write
    for (int i = 0; i < 1<<n; i++)
    {
        amp->real = i * 1.1; // Example data
        amp->imag = i * 2.2;
        ssize_t written = pwrite(fd, amp, sizeof(Complex), i * sizeof(Complex));

        if (written != sizeof(Complex))
        {
            perror("Write error");
            free(amp);
            close(fd);
            return 1;
        }
    }
    
    // read
    printf("after write\n");
    for (int i = 0; i < 1<<n; i++)
    {
        pread(fd, amp, sizeof(Complex), i * sizeof(Complex));
        printf("Statevec real: %f, imag: %f\n", amp->real, amp->imag);
    }
    */
    free(amp);
    close(fd);
    

    return 0;
}
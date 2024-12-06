#include <cmath>
#include <string>
#include <fstream>
#include <iostream>

#include "fcntl.h"
#include "unistd.h"
#include <omp.h>

#include "state.hpp"
#include "circuit.hpp"

using namespace std;

void rx_mem(int n)
{
    State state;
    state.numQubitTotal = n;
    createState(state);
    for (int i = 0; i < n; i++)
        rxGate(state, M_PI_4, i);
    ofstream file;
    file.open("./rx.mem", ios::binary | ios::out);
    file.write((char *)state.stateVec, state.numAmpTotal * sizeof(Complex));
    file.close();
    destroyState(state);
}

void rx_raid(int n)
{
    State state;
    state.numQubitTotal = n;
    createState(state);

    // file initialization
    int fd = open("./state", O_RDWR | O_CREAT, 0777);
    // allocate storage
    if (ftruncate(fd, state.numAmpTotal * sizeof(Complex)) != 0)
    {
        perror("Failed to allocate space");
        close(fd);
    }
    // init first with one, rest with zero
    Complex zero = {0.0f, 0.0f}, one = {1.0f, 0.0f};
    pwrite(fd, &one, sizeof(Complex), 0);
    for (int i = 1; i < state.numAmpTotal; i++)
    {
        pwrite(fd, &zero, sizeof(Complex), i * sizeof(Complex));
    }
    close(fd);

    // for (int i = 0; i < state.numAmpTotal; i++)
    // {
    //     Complex amp;
    //     pread(fd, &amp, sizeof(Complex), i * sizeof(Complex));
    //     printf("Statevec real: %f, imag: %f\n", amp.real, amp.imag);
    // }

    for (int i = 0; i < n; i++)
        rxGateRaid(state, M_PI_4, i);
    ofstream file;
    file.open("./rx.raid", ios::binary | ios::out);
    file.write((char *)state.stateVec, state.numAmpTotal * sizeof(Complex));
    file.close();
    destroyState(state);
}

void parallel_write(int n)
{
    State state;
    state.numQubitTotal = n;
    createState(state);
    createShardedStateFiles(state);

    return;
}

void hw1_3_1(int n)
{
    State state;
    state.numQubitTotal = n;
    createState(state);
    for (int i = 0; i < n; i++)
        rxGate(state, M_PI_4, i);
    ofstream file;
    file.open("./rx.txt", ios::binary | ios::out);
    file.write((char *)state.stateVec, state.numAmpTotal * sizeof(Complex));
    file.close();
    destroyState(state);
}

void hw1_3_2(int n)
{
    State state;
    state.numQubitTotal = n;
    createState(state);
    // TODO: Apply h-gates
    for (int i = 0; i < n; i++)
    {
        hGate(state, i);
    }
    ofstream file;
    file.open("./h.txt", ios::binary | ios::out);
    file.write((char *)state.stateVec, state.numAmpTotal * sizeof(Complex));
    file.close();
    destroyState(state);
}

void hw1_3_3(int n)
{
    double angle = M_PI_4;
    State state;
    state.numQubitTotal = n;
    createState(state);
    // TODO: Apply h-gates and cp-gates
    for (int i = 0; i < n; i++)
    {
        hGate(state, i);
    }
    cpGate(state, angle, std::vector<int>{0, 1});
    cpGate(state, angle, std::vector<int>{0, 2});
    ofstream file;
    file.open("./cp.txt", ios::binary | ios::out);
    file.write((char *)state.stateVec, state.numAmpTotal * sizeof(Complex));
    file.close();
    destroyState(state);
}

void hw1_3_4(int n)
{
    State state;
    state.numQubitTotal = n;
    createState(state);
    // TODO: Apply a QFT circuit
    hGate(state, 0);
    cpGate(state, M_PI_2, std::vector<int>{0, 1});
    cpGate(state, M_PI_4, std::vector<int>{0, 2});
    hGate(state, 1);
    cpGate(state, M_PI_2, std::vector<int>{1, 2});
    hGate(state, 2);

    ofstream file;
    file.open("./qft.txt", ios::binary | ios::out);
    file.write((char *)state.stateVec, state.numAmpTotal * sizeof(Complex));
    file.close();
    destroyState(state);
}

int main(int argc, char *argv[])
{
    int n = 20;
    // hw1_3_1(n);
    // hw1_3_2(n);
    // hw1_3_3(n);
    // hw1_3_4(n);
    // rx_mem(n);
    rx_raid(n);
    // parallel_write(n);

    // #pragma omp parallel for
    //     for (int i = 0; i < 1000000; i++)
    //     {
    //         std::cout << i << std::endl;
    //     }

    return 0;
}
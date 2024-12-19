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
    Complex one = {1.0f, 0.0f}, zero = {0.0f, 0.0f};
    pwrite(fd, &one, sizeof(Complex), 0);
    for (int i = 1; i < state.numAmpTotal; i++)
    {
        pwrite(fd, &zero, sizeof(Complex), i * sizeof(Complex));
    }
    close(fd);

    for (int i = 0; i < n; i++)
        rxGateRaid(state, M_PI_4, i);
    ofstream file;
    file.open("./rx.raid", ios::binary | ios::out);
    file.write((char *)state.stateVec, state.numAmpTotal * sizeof(Complex));
    file.close();
    destroyState(state);
}

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

void h_mem(int n)
{
    State state;
    state.numQubitTotal = n;
    createState(state);

    for (int i = 0; i < n; i++)
        hGate(state, i);

    ofstream file;
    file.open("./h.mem", ios::binary | ios::out);
    file.write((char *)state.stateVec, state.numAmpTotal * sizeof(Complex));
    file.close();
    destroyState(state);
}

void h_raid_chunk(int n)
{
    ManagedStateVector state(n, "/mnt/raid/states/shard", "h.raid");

    for (int i = 0; i < n; i++)
    {
        hGateRaid(state, i);
    }
    state.finish_task();
}

/* original chunk implementation
void rx_raid_chunk(int n)
{
    State state;
    state.numQubitTotal = n;
    createState(state);

    // file initialization
    createShardedStateFiles(state, CHUNK_SIZE);

    for (int i = 0; i < n; i++)
        rxGateRaid(state, M_PI_4, i);

    concatShardedStateFiles(std::string("rx.raid"), state.numAmpTotal >> EXPONENT, CHUNK_SIZE);
    // ofstream file;
    // file.open("./rx.raid", ios::binary | ios::out);
    // file.write((char *)state.stateVec, state.numAmpTotal * sizeof(Complex));
    // file.close();
    destroyState(state);
}
*/

void rx_raid_chunk(int n)
{
    ManagedStateVector state(n, "/mnt/raid/states/shard", "rx.raid");

    for (int i = 0; i < n; i++)
    {
        rxGateRaid2(state, M_PI_4, i);
    }
    state.finish_task();
}

void cp_mem(int n)
{
    State state;
    state.numQubitTotal = n;
    createState(state);

    for (int i = 0; i < n; i++)
        rxGate(state, M_PI_4, i);
    cpGate(state, M_PI_2, std::vector<int>{0, 1});

    ofstream file;
    file.open("./cp.mem", ios::binary | ios::out);
    file.write((char *)state.stateVec, state.numAmpTotal * sizeof(Complex));
    file.close();
    destroyState(state);
}

// original cp_raid_chunk impl.
// void cp_raid_chunk(int n)
// {
//     State state;
//     state.numQubitTotal = n;
//     createState(state);

//     // file initialization
//     createShardedStateFiles(state, CHUNK_SIZE);

//     for (int i = 0; i < n; i++)
//         rxGateRaid(state, M_PI_4, i);
//     cpGateRaid(state, M_PI_2, std::vector<int>{0, 1});

//     concatShardedStateFiles(std::string("cp.raid"), state.numAmpTotal >> EXPONENT, CHUNK_SIZE);
//     destroyState(state);
// }
void cp_raid_chunk(int n)
{
    ManagedStateVector state(n, "/mnt/raid/states/shard", "cp.raid");

    for (int i = 0; i < n; i++)
        rxGateRaid2(state, M_PI_4, i);
    cpGateRaid2(state, M_PI_2, std::vector<int>{0, 1});

    state.finish_task();
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
    int n = 30;
    // hw1_3_1(n);
    // hw1_3_2(n);
    // hw1_3_3(n);
    // hw1_3_4(n);
    // rx_mem(n);
    // rx_raid_chunk(n);
    // h_mem(n);
    // h_raid_chunk(n);
    // cp_mem(n);
    cp_raid_chunk(n);
    // parallel_write(n);

    // #pragma omp parallel for
    //     for (int i = 0; i < 1000000; i++)
    //     {
    //         std::cout << i << std::endl;
    //     }

    return 0;
}
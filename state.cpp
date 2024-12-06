#include "state.hpp"

#include "fcntl.h"
#include "unistd.h"

#include <sstream>
#include <iostream>
#include <omp.h>

void createState(State &state)
{
    state.numAmpTotal = 1ull << state.numQubitTotal;
    state.stateVec = (Complex *)calloc(state.numAmpTotal, sizeof(Complex));
    memset(state.stateVec, 0, state.numAmpTotal * sizeof(Complex));
    state.stateVec[0].real = 1.0f;
    return;
}

void createSingleStateFile(State &state)
{
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
    return;
}

void createShardedStateFiles(State &state)
{

    Complex zero = {0.0f, 0.0f}, one = {1.0f, 0.0f};

#pragma omp parallel for num_threads(32)
    for (int i = 0; i < 32; i++)
    {
        std::ostringstream filename;
        filename << "/mnt/raid/states/s" << i;
        // std::cout << filename.str();
        // file initialization
        int fd = open(filename.str().c_str(), O_RDWR | O_CREAT, 0777);
        // allocate storage
        if (ftruncate(fd, state.numAmpTotal / 32 * sizeof(Complex)) != 0)
        {
            perror("Failed to allocate space");
            close(fd);
        }
        // init first with one, rest with zero
        if (i == 0)
            pwrite(fd, &one, sizeof(Complex), 0);
        else
            pwrite(fd, &zero, sizeof(Complex), 0);
        for (int j = 1; j < state.numAmpTotal; j++)
        {
            pwrite(fd, &zero, sizeof(Complex), j * sizeof(Complex));
        }
        close(fd);
    }
    return;
}

void printState(const State &state, int num)
{
    for (int i = 0; i < num; i++)
    {
        printf("Statevec idx: %d, real: %f, imag: %f\n", i, state.stateVec[i].real, state.stateVec[i].imag);
    }
    return;
}

void destroyState(const State &state)
{
    free(state.stateVec);
    return;
}

// ManagedStateVector::ManagedStateVector()
// {
//     // fd = open();
// }

void ManagedStateVector::test()
{
    return;
}
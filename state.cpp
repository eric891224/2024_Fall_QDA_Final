#include "state.hpp"

void createState(State &state)
{
    state.numAmpTotal = 1ull << state.numQubitTotal;
    state.stateVec = (Complex *)calloc(state.numAmpTotal, sizeof(Complex));
    memset(state.stateVec, 0, state.numAmpTotal * sizeof(Complex));
    state.stateVec[0].real = 1.0f;
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
// }

void ManagedStateVector::test()
{
    return;
}
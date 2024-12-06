#ifndef _STATE_HPP_
#define _STATE_HPP_

#include <cstdlib>
#include <cstring>
#include <cstdio>

typedef unsigned long long ull;

typedef struct
{
    double real;
    double imag;
} Complex;

typedef struct
{
    Complex *stateVec;
    int numQubitTotal;
    ull numAmpTotal;
} State;

class ManagedStateVector
{
public:
    int fd;
    void test();
};

void createState(State &state);
void createShardedStateFiles(State &state);
void printState(const State &state, int num);
void destroyState(const State &state);

#endif

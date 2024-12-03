#include <cmath>
#include <string>
#include <fstream>
#include <iostream>

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
    for (int i = 0; i < n; i++)
        rxGateRaid(state, M_PI_4, i);
    ofstream file;
    file.open("./rx.raid", ios::binary | ios::out);
    file.write((char *)state.stateVec, state.numAmpTotal * sizeof(Complex));
    file.close();
    destroyState(state);
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
    int n = 3;
    // hw1_3_1(n);
    // hw1_3_2(n);
    // hw1_3_3(n);
    // hw1_3_4(n);
    rx_mem(n);
    // rx_raid(n);

    return 0;
}
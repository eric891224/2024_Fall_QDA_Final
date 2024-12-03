#ifndef _CIRCUIT_HPP_
#define _CIRCUIT_HPP_

#include <vector>
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include "state.hpp"

ull insert_bit_0(ull task, const char targ);
ull insert_bit_1(ull task, const char targ);
void rxGate(State &state, const double angle, const int targQubit);
void rxGateRaid(State &state, const double angle, const int targQubit);

void hGate(State &state, const int targQubit);
void cpGate(State &state, const double angle, std::vector<int> targQubits);

#endif
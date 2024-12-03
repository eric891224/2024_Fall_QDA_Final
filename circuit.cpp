#include "circuit.hpp"

ull insert_bit_0(ull task, const char targ)
{
    ull mask = (1ULL << targ) - 1;
    return ((task >> targ) << (targ + 1)) | (task & mask);
}

ull insert_bit_1(ull task, const char targ)
{
    ull mask = (1ULL << targ) - 1;
    return ((task >> targ) << (targ + 1)) | (task & mask) | (1ULL << targ);
}

void rxGate(State &state, const double angle, const int targQubit)
{
    double sinAngle, cosAngle;
    sincosf64(angle / 2, &sinAngle, &cosAngle);
    // pragma
    for (ull i = 0; i < state.numAmpTotal >> 1; i++)
    {
        ull up_off = insert_bit_0(i, targQubit);
        ull lo_off = up_off + (1ull << targQubit);

        Complex up = state.stateVec[up_off];
        Complex lo = state.stateVec[lo_off];

        state.stateVec[up_off].real = up.real * cosAngle + lo.imag * sinAngle;
        state.stateVec[up_off].imag = up.imag * cosAngle - lo.real * sinAngle;
        state.stateVec[lo_off].real = up.imag * sinAngle + lo.real * cosAngle;
        state.stateVec[lo_off].imag = -up.real * sinAngle + lo.imag * cosAngle;
    }
}

void rxGateRaid(State &state, const double angle, const int targQubit)
{
    double sinAngle, cosAngle;
    sincosf64(angle / 2, &sinAngle, &cosAngle);
    // pragma
    for (ull i = 0; i < state.numAmpTotal >> 1; i++)
    {
        ull up_off = insert_bit_0(i, targQubit);
        ull lo_off = up_off + (1ull << targQubit);

        // TODO: read
        // TODO: compute
        // TODO: write
    }
}

void hGate(State &state, const int targQubit)
{
    const double coeff = 1 / sqrt(2.f);
    for (ull i = 0; i < state.numAmpTotal >> 1; i++)
    {
        ull up_off = insert_bit_0(i, targQubit);
        ull lo_off = up_off + (1ull << targQubit);

        Complex up = state.stateVec[up_off];
        Complex lo = state.stateVec[lo_off];

        // TODO: Implement h-gates
        state.stateVec[up_off].real = up.real * coeff + lo.real * coeff;
        state.stateVec[up_off].imag = up.imag * coeff + lo.imag * coeff;
        state.stateVec[lo_off].real = up.real * coeff - lo.real * coeff;
        state.stateVec[lo_off].imag = up.imag * coeff - lo.imag * coeff;
    }
}

void cpGate(State &state, const double angle, std::vector<int> targQubits)
{
    const int close = 2;
    double sinAngle, cosAngle;
    sincosf64(angle, &sinAngle, &cosAngle);
    std::sort(targQubits.begin(), targQubits.end());
    for (ull i = 0; i < state.numAmpTotal >> close; i++)
    {
        ull idx = insert_bit_1(i, targQubits[0]);
        idx = insert_bit_1(idx, targQubits[1]);

        Complex q = state.stateVec[idx], qOut;

        // TODO: Implement cp-gates
        qOut.real = q.real * cosAngle - q.imag * sinAngle;
        qOut.imag = q.real * sinAngle + q.imag * cosAngle;
        state.stateVec[idx] = qOut;
    }
}
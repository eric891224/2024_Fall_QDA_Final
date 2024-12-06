#include "circuit.hpp"

#include "fcntl.h"
#include "unistd.h"
#include <omp.h>

int NUM_THREADS = 1;
int EXPONENT = 10;
int CHUNK_SIZE = 1 << EXPONENT;

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

    // file initialization
    int fd = open("./state", O_RDWR);

    // Complex up, lo;
    // Complex *new_up = (Complex *)calloc(1, sizeof(Complex)), *new_lo = (Complex *)calloc(1, sizeof(Complex));

    Complex *amps_1 = (Complex *)calloc(NUM_THREADS * CHUNK_SIZE, sizeof(Complex));
    Complex *amps_2 = (Complex *)calloc(NUM_THREADS * CHUNK_SIZE, sizeof(Complex));
    Complex *new_amps_1 = (Complex *)calloc(NUM_THREADS * CHUNK_SIZE, sizeof(Complex));
    Complex *new_amps_2 = (Complex *)calloc(NUM_THREADS * CHUNK_SIZE, sizeof(Complex));

    int num_tasks = state.numAmpTotal / (CHUNK_SIZE * 2);
    int group_size = 1 << (targQubit + 1);

    for (ull t = 0; t < num_tasks; t++)
    {
        // calculate chunk index
        int tid = 0;                                                           // omp_get_thread_num() when using openmp
        int pattern = (targQubit - EXPONENT) < 0 ? 0 : (targQubit - EXPONENT); // equivalent qbit pattern for insert bit 0 function
        int chunk_1_id = insert_bit_0(t, pattern);
        int chunk_2_id = chunk_1_id + (1ull << pattern);

        // read
        pread(fd, &(amps_1[tid * CHUNK_SIZE]), CHUNK_SIZE * sizeof(Complex), chunk_1_id * CHUNK_SIZE * sizeof(Complex));
        pread(fd, &(amps_2[tid * CHUNK_SIZE]), CHUNK_SIZE * sizeof(Complex), chunk_2_id * CHUNK_SIZE * sizeof(Complex));

        // computation
        if (group_size < CHUNK_SIZE)
        {
            for (int compute_id = 0; compute_id < CHUNK_SIZE >> 1; compute_id++)
            {
                ull up_off = insert_bit_0(compute_id, targQubit);
                ull lo_off = up_off + (1ull << targQubit);

                up_off += tid * CHUNK_SIZE; // tid * CHUNK_SIZE + up_off
                lo_off += tid * CHUNK_SIZE; // tid * CHUNK_SIZE + lo_off
                // first chunk
                new_amps_1[up_off].real = amps_1[up_off].real * cosAngle + amps_1[lo_off].imag * sinAngle;
                new_amps_1[up_off].imag = amps_1[up_off].imag * cosAngle - amps_1[lo_off].real * sinAngle;
                new_amps_1[lo_off].real = amps_1[up_off].imag * sinAngle + amps_1[lo_off].real * cosAngle;
                new_amps_1[lo_off].imag = -amps_1[up_off].real * sinAngle + amps_1[lo_off].imag * cosAngle;
                // second chunk
                new_amps_2[up_off].real = amps_2[up_off].real * cosAngle + amps_2[lo_off].imag * sinAngle;
                new_amps_2[up_off].imag = amps_2[up_off].imag * cosAngle - amps_2[lo_off].real * sinAngle;
                new_amps_2[lo_off].real = amps_2[up_off].imag * sinAngle + amps_2[lo_off].real * cosAngle;
                new_amps_2[lo_off].imag = -amps_2[up_off].real * sinAngle + amps_2[lo_off].imag * cosAngle;
            }
        }
        else
        {
            for (int i = 0; i < CHUNK_SIZE; i++)
            {
                ull up_off = i + tid * CHUNK_SIZE;
                ull lo_off = up_off;
                new_amps_1[up_off].real = amps_1[up_off].real * cosAngle + amps_2[lo_off].imag * sinAngle;
                new_amps_1[up_off].imag = amps_1[up_off].imag * cosAngle - amps_2[lo_off].real * sinAngle;
                new_amps_2[lo_off].real = amps_1[up_off].imag * sinAngle + amps_2[lo_off].real * cosAngle;
                new_amps_2[lo_off].imag = -amps_1[up_off].real * sinAngle + amps_2[lo_off].imag * cosAngle;
            }
        }

        // write back
        pwrite(fd, &(new_amps_1[tid * CHUNK_SIZE]), CHUNK_SIZE * sizeof(Complex), chunk_1_id * CHUNK_SIZE * sizeof(Complex));
        pwrite(fd, &(new_amps_2[tid * CHUNK_SIZE]), CHUNK_SIZE * sizeof(Complex), chunk_2_id * CHUNK_SIZE * sizeof(Complex));
    }

    free(amps_1);
    free(amps_2);
    free(new_amps_1);
    free(new_amps_2);

    // #pragma omp parallel for num_threads(16)
    //     for (ull i = 0; i < state.numAmpTotal >> 1; i++)
    //     {
    //         // Complex up, lo;
    //         // Complex *new_up = (Complex *)calloc(1, sizeof(Complex)), *new_lo = (Complex *)calloc(1, sizeof(Complex));

    //         ull up_off = insert_bit_0(i, targQubit);
    //         ull lo_off = up_off + (1ull << targQubit);

    //         // TODO: read
    //         pread(fd, &up, sizeof(Complex), up_off * sizeof(Complex));
    //         pread(fd, &lo, sizeof(Complex), lo_off * sizeof(Complex));

    //         // TODO: compute
    //         new_up->real = up.real * cosAngle + lo.imag * sinAngle;
    //         new_up->imag = up.imag * cosAngle - lo.real * sinAngle;
    //         new_lo->real = up.imag * sinAngle + lo.real * cosAngle;
    //         new_lo->imag = -up.real * sinAngle + lo.imag * cosAngle;

    //         // TODO: write
    //         pwrite(fd, new_up, sizeof(Complex), up_off * sizeof(Complex));
    //         pwrite(fd, new_lo, sizeof(Complex), lo_off * sizeof(Complex));

    //         free(new_up);
    //         free(new_lo);
    //     }

    // free(new_up);
    // free(new_lo);
    close(fd);
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
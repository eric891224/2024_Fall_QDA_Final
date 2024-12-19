#include "state.hpp"
#include "circuit.hpp"

#include "fcntl.h"
#include "unistd.h"
#include <omp.h>
#include <iostream>
#include <sstream>

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

void copy_amps(Complex *amps1, Complex *amps2)
{
    // use memcpy instead?
    for (int i = 0; i < CHUNK_SIZE; i++)
    {
        amps1[i] = amps2[i];
    }
}

void rxGate(State &state, const double angle, const int targQubit)
{
    double sinAngle, cosAngle;
    sincosf64(angle / 2, &sinAngle, &cosAngle);
#pragma omp parallel for num_threads(NUM_THREADS)
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

    Complex **amps_1 = (Complex **)calloc(NUM_THREADS, sizeof(Complex *));
    Complex **amps_2 = (Complex **)calloc(NUM_THREADS, sizeof(Complex *));
    Complex **new_amps_1 = (Complex **)calloc(NUM_THREADS, sizeof(Complex *));
    Complex **new_amps_2 = (Complex **)calloc(NUM_THREADS, sizeof(Complex *));

#pragma omp parallel for num_threads(NUM_THREADS)
    for (int i = 0; i < NUM_THREADS; i++)
    {
        amps_1[i] = (Complex *)calloc(CHUNK_SIZE, sizeof(Complex));
        amps_2[i] = (Complex *)calloc(CHUNK_SIZE, sizeof(Complex));
        new_amps_1[i] = (Complex *)calloc(CHUNK_SIZE, sizeof(Complex));
        new_amps_2[i] = (Complex *)calloc(CHUNK_SIZE, sizeof(Complex));
    }

    int num_tasks = state.numAmpTotal / (CHUNK_SIZE * 2);
    int group_size = 1 << (targQubit + 1);

#pragma omp parallel for num_threads(NUM_THREADS)
    for (ull t = 0; t < num_tasks; t++)
    {
        // int tid = 0;                                                           // omp_get_thread_num() when using openmp
        int tid = omp_get_thread_num();

        // calculate chunk index
        int pattern = (targQubit - EXPONENT) < 0 ? 0 : (targQubit - EXPONENT); // equivalent qbit pattern for insert bit 0 function
        ull chunk_1_id = insert_bit_0(t, pattern);
        ull chunk_2_id = chunk_1_id + (1ull << pattern);

        std::ostringstream sharded_filename_1, sharded_filename_2;
        sharded_filename_1 << "./shard" << chunk_1_id;
        sharded_filename_2 << "./shard" << chunk_2_id;
        int chunk_1_fd = open(sharded_filename_1.str().c_str(), O_RDWR);
        int chunk_2_fd = open(sharded_filename_2.str().c_str(), O_RDWR);

        // read
        pread(chunk_1_fd, amps_1[tid], CHUNK_SIZE * sizeof(Complex), 0);
        pread(chunk_2_fd, amps_2[tid], CHUNK_SIZE * sizeof(Complex), 0);

        // computation
        if (group_size <= CHUNK_SIZE)
        {
            for (ull compute_id = 0; compute_id < CHUNK_SIZE >> 1; compute_id++)
            {
                ull up_off = insert_bit_0(compute_id, targQubit);
                ull lo_off = up_off + (1ull << targQubit);

                // up_off += tid * CHUNK_SIZE; // tid * CHUNK_SIZE + up_off
                // lo_off += tid * CHUNK_SIZE; // tid * CHUNK_SIZE + lo_off

                // first chunk
                new_amps_1[tid][up_off].real = amps_1[tid][up_off].real * cosAngle + amps_1[tid][lo_off].imag * sinAngle;
                new_amps_1[tid][up_off].imag = amps_1[tid][up_off].imag * cosAngle - amps_1[tid][lo_off].real * sinAngle;
                new_amps_1[tid][lo_off].real = amps_1[tid][up_off].imag * sinAngle + amps_1[tid][lo_off].real * cosAngle;
                new_amps_1[tid][lo_off].imag = -amps_1[tid][up_off].real * sinAngle + amps_1[tid][lo_off].imag * cosAngle;
                // second chunk
                new_amps_2[tid][up_off].real = amps_2[tid][up_off].real * cosAngle + amps_2[tid][lo_off].imag * sinAngle;
                new_amps_2[tid][up_off].imag = amps_2[tid][up_off].imag * cosAngle - amps_2[tid][lo_off].real * sinAngle;
                new_amps_2[tid][lo_off].real = amps_2[tid][up_off].imag * sinAngle + amps_2[tid][lo_off].real * cosAngle;
                new_amps_2[tid][lo_off].imag = -amps_2[tid][up_off].real * sinAngle + amps_2[tid][lo_off].imag * cosAngle;
            }
        }
        else
        {
            for (ull i = 0; i < CHUNK_SIZE; i++)
            {
                // ull up_off = i + tid * CHUNK_SIZE;
                ull up_off = i;
                ull lo_off = up_off;

                new_amps_1[tid][up_off].real = amps_1[tid][up_off].real * cosAngle + amps_2[tid][lo_off].imag * sinAngle;
                new_amps_1[tid][up_off].imag = amps_1[tid][up_off].imag * cosAngle - amps_2[tid][lo_off].real * sinAngle;
                new_amps_2[tid][lo_off].real = amps_1[tid][up_off].imag * sinAngle + amps_2[tid][lo_off].real * cosAngle;
                new_amps_2[tid][lo_off].imag = -amps_1[tid][up_off].real * sinAngle + amps_2[tid][lo_off].imag * cosAngle;
            }
        }

        // write back
        pwrite(chunk_1_fd, new_amps_1[tid], CHUNK_SIZE * sizeof(Complex), 0);
        pwrite(chunk_2_fd, new_amps_2[tid], CHUNK_SIZE * sizeof(Complex), 0);

        close(chunk_1_fd);
        close(chunk_2_fd);
    }

    for (int i = 0; i < NUM_THREADS; i++)
    {
        free(amps_1[i]);
        free(amps_2[i]);
        free(new_amps_1[i]);
        free(new_amps_2[i]);
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
}

void rxGateRaid2(ManagedStateVector &state, const double angle, const int targQubit)
{
    double sinAngle, cosAngle;
    sincosf64(angle / 2, &sinAngle, &cosAngle);

    ull num_tasks = state.numAmpTotal >> (EXPONENT + 1);
    ull group_size = 1 << (targQubit + 1);

#pragma omp parallel for num_threads(NUM_THREADS)
    for (ull t = 0; t < num_tasks; t++)
    {
        int tid = omp_get_thread_num();

        // calculate chunk index
        int pattern = (targQubit - EXPONENT) < 0 ? 0 : (targQubit - EXPONENT); // equivalent qbit pattern for insert bit 0 function
        ull chunk_1_id = insert_bit_0(t, pattern);
        ull chunk_2_id = chunk_1_id + (1ull << pattern);

        // read chunks
        Complex *amps_1 = state.get_chunk_by_id(chunk_1_id);
        Complex *amps_2 = state.get_chunk_by_id(chunk_2_id);
        Complex *new_amps_1 = state.new_amps_buffer1[tid];
        Complex *new_amps_2 = state.new_amps_buffer2[tid];

        // computation
        if (group_size <= CHUNK_SIZE)
        {
            for (ull compute_id = 0; compute_id < CHUNK_SIZE >> 1; compute_id++)
            {
                ull up_off = insert_bit_0(compute_id, targQubit);
                ull lo_off = up_off + (1ull << targQubit);

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
            for (ull i = 0; i < CHUNK_SIZE; i++)
            {
                ull up_off = i;
                ull lo_off = up_off;

                new_amps_1[up_off].real = amps_1[up_off].real * cosAngle + amps_2[lo_off].imag * sinAngle;
                new_amps_1[up_off].imag = amps_1[up_off].imag * cosAngle - amps_2[lo_off].real * sinAngle;
                new_amps_2[lo_off].real = amps_1[up_off].imag * sinAngle + amps_2[lo_off].real * cosAngle;
                new_amps_2[lo_off].imag = -amps_1[up_off].real * sinAngle + amps_2[lo_off].imag * cosAngle;
            }
        }

        // update amps, swapping their address will do
        // copy_amps(amps_1, new_amps_1);
        // copy_amps(amps_2, new_amps_2);
        state.update_amps_1(chunk_1_id, tid);
        state.update_amps_2(chunk_2_id, tid);

        // mark finished
        state.push_chunk_to_stack(chunk_1_id);
        state.push_chunk_to_stack(chunk_2_id);
    }
}

void hGate(State &state, const int targQubit)
{
    const double coeff = 1 / sqrt(2.f);
#pragma omp parallel for num_threads(NUM_THREADS)
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

void hGateRaid(ManagedStateVector &state, const int targQubit)
{
    const double coeff = 1 / sqrt(2.f);

    ull num_tasks = state.numAmpTotal >> (EXPONENT + 1);
    ull group_size = 1 << (targQubit + 1);

#pragma omp parallel for num_threads(NUM_THREADS)
    for (ull t = 0; t < num_tasks; t++)
    {
        int tid = omp_get_thread_num();

        // calculate chunk index
        int pattern = (targQubit - EXPONENT) < 0 ? 0 : (targQubit - EXPONENT); // equivalent qbit pattern for insert bit 0 function
        ull chunk_1_id = insert_bit_0(t, pattern);
        ull chunk_2_id = chunk_1_id + (1ull << pattern);

        // read chunks
        Complex *amps_1 = state.get_chunk_by_id(chunk_1_id);
        Complex *amps_2 = state.get_chunk_by_id(chunk_2_id);
        Complex *new_amps_1 = state.new_amps_buffer1[tid];
        Complex *new_amps_2 = state.new_amps_buffer2[tid];

        // computation
        if (group_size <= CHUNK_SIZE)
        {
            for (ull compute_id = 0; compute_id < CHUNK_SIZE >> 1; compute_id++)
            {
                ull up_off = insert_bit_0(compute_id, targQubit);
                ull lo_off = up_off + (1ull << targQubit);

                // first chunk
                new_amps_1[up_off].real = amps_1[up_off].real * coeff + amps_1[lo_off].real * coeff;
                new_amps_1[up_off].imag = amps_1[up_off].imag * coeff + amps_1[lo_off].imag * coeff;
                new_amps_1[lo_off].real = amps_1[up_off].real * coeff - amps_1[lo_off].real * coeff;
                new_amps_1[lo_off].imag = amps_1[up_off].imag * coeff - amps_1[lo_off].imag * coeff;
                // second chunk
                new_amps_2[up_off].real = amps_2[up_off].real * coeff + amps_2[lo_off].real * coeff;
                new_amps_2[up_off].imag = amps_2[up_off].imag * coeff + amps_2[lo_off].imag * coeff;
                new_amps_2[lo_off].real = amps_2[up_off].real * coeff - amps_2[lo_off].real * coeff;
                new_amps_2[lo_off].imag = amps_2[up_off].imag * coeff - amps_2[lo_off].imag * coeff;
            }
        }
        else
        {
            for (ull i = 0; i < CHUNK_SIZE; i++)
            {
                ull up_off = i;
                ull lo_off = up_off;

                new_amps_1[up_off].real = amps_1[up_off].real * coeff + amps_2[lo_off].real * coeff;
                new_amps_1[up_off].imag = amps_1[up_off].imag * coeff + amps_2[lo_off].imag * coeff;
                new_amps_2[lo_off].real = amps_1[up_off].real * coeff - amps_2[lo_off].real * coeff;
                new_amps_2[lo_off].imag = amps_1[up_off].imag * coeff - amps_2[lo_off].imag * coeff;
            }
        }

        // update amps, swapping their address will do
        state.update_amps_1(chunk_1_id, tid);
        state.update_amps_2(chunk_2_id, tid);

        // mark finished
        state.push_chunk_to_stack(chunk_1_id);
        state.push_chunk_to_stack(chunk_2_id);
    }
}

void cpGate(State &state, const double angle, std::vector<int> targQubits)
{
    const int close = 2;
    double sinAngle, cosAngle;
    sincosf64(angle, &sinAngle, &cosAngle);
    std::sort(targQubits.begin(), targQubits.end());

#pragma omp parallel for num_threads(NUM_THREADS)
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

void cpGateRaid(State &state, const double angle, std::vector<int> targQubits)
{
    const int off = 2;
    double sinAngle, cosAngle;
    sincosf64(angle, &sinAngle, &cosAngle);
    std::sort(targQubits.begin(), targQubits.end());

    Complex **amps_1 = (Complex **)calloc(NUM_THREADS, sizeof(Complex *));
    Complex **new_amps_1 = (Complex **)calloc(NUM_THREADS, sizeof(Complex *));

    for (int i = 0; i < NUM_THREADS; i++)
    {
        amps_1[i] = (Complex *)calloc(CHUNK_SIZE, sizeof(Complex));
        new_amps_1[i] = (Complex *)calloc(CHUNK_SIZE, sizeof(Complex));
    }

    // int group_size = 1 << (targQubits[1] + 1);
    ull num_chunk = state.numAmpTotal >> EXPONENT;

    std::vector<std::vector<ull>> v;
    ull pre_chunk_id = num_chunk;

    for (ull i = 0; i < state.numAmpTotal >> off; i++)
    {
        ull idx = insert_bit_1(i, targQubits[0]);
        idx = insert_bit_1(idx, targQubits[1]);
        // std::cout << idx << std::endl;

        // if (i >= 10)
        //     break;

        ull cur_chunk_id = idx >> EXPONENT;
        if (cur_chunk_id != pre_chunk_id)
        {
            pre_chunk_id = cur_chunk_id;
            v.push_back(std::vector<ull>{idx});
        }
        else
            v.back().push_back(idx);

        // std::cout << v.back().back() << std::endl;
        // if (i >= 10)
        //     break;
    }

#pragma omp parallel for num_threads(NUM_THREADS)
    for (ull i = 0; i < v.size(); i++)
    {
        int tid = omp_get_thread_num();
        ull chunk_id = v[i][0] >> EXPONENT;

        std::ostringstream sharded_filename;
        sharded_filename << "./shard" << chunk_id;
        int chunk_fd = open(sharded_filename.str().c_str(), O_RDWR);

        // read
        pread(chunk_fd, amps_1[tid], CHUNK_SIZE * sizeof(Complex), 0);
        pread(chunk_fd, new_amps_1[tid], CHUNK_SIZE * sizeof(Complex), 0);

        // computation
        for (int j = 0; j < v[i].size(); j++)
        {
            ull global_idx = v[i][j];
            ull local_idx = global_idx % CHUNK_SIZE;

            // std::cout << local_idx << std::endl;
            // std::cout << "chunk id" << chunk_id << std::endl;

            new_amps_1[tid][local_idx].real = amps_1[tid][local_idx].real * cosAngle - amps_1[tid][local_idx].imag * sinAngle;
            new_amps_1[tid][local_idx].imag = amps_1[tid][local_idx].real * sinAngle + amps_1[tid][local_idx].imag * cosAngle;
            // std::cout << new_amps_1[tid][local_idx].real << std::endl;
        }

        // write back
        pwrite(chunk_fd, new_amps_1[tid], CHUNK_SIZE * sizeof(Complex), 0);

        close(chunk_fd);
    }

    for (int i = 0; i < NUM_THREADS; i++)
    {
        free(amps_1[i]);
        free(new_amps_1[i]);
    }
    free(amps_1);
    free(new_amps_1);
    // for (ull i = 0; i < state.numAmpTotal >> close; i++)
    // {
    //     ull idx = insert_bit_1(i, targQubits[0]);
    //     idx = insert_bit_1(idx, targQubits[1]);

    //     Complex q = state.stateVec[idx], qOut;

    //     // TODO: Implement cp-gates
    //     qOut.real = q.real * cosAngle - q.imag * sinAngle;
    //     qOut.imag = q.real * sinAngle + q.imag * cosAngle;
    //     state.stateVec[idx] = qOut;
    // }
}

void cpGateRaid2(ManagedStateVector &state, const double angle, std::vector<int> targQubits)
{
    const int off = 2;
    double sinAngle, cosAngle;
    sincosf64(angle, &sinAngle, &cosAngle);
    std::sort(targQubits.begin(), targQubits.end());

    ull num_chunk = state.numAmpTotal >> EXPONENT;

    std::vector<std::vector<ull>> v;
    ull pre_chunk_id = num_chunk;

#pragma omp parallel for num_threads(NUM_THREADS)
    for (ull i = 0; i < state.numAmpTotal >> off; i++)
    {
        ull idx = insert_bit_1(i, targQubits[0]);
        idx = insert_bit_1(idx, targQubits[1]);

        ull cur_chunk_id = idx >> EXPONENT;
#pragma omp critical(task)
        {
            if (cur_chunk_id != pre_chunk_id)
            {
                pre_chunk_id = cur_chunk_id;
                v.push_back(std::vector<ull>{idx});
            }
            else
                v.back().push_back(idx);
        }
    }

#pragma omp parallel for num_threads(NUM_THREADS)
    for (ull i = 0; i < v.size(); i++)
    {
        int tid = omp_get_thread_num();
        ull chunk_id = v[i][0] >> EXPONENT;

        // read chunks
        Complex *amps_1 = state.get_chunk_by_id(chunk_id);
        Complex *new_amps_1 = state.new_amps_buffer1[tid];

        // computation
        for (int j = 0; j < v[i].size(); j++)
        {
            ull global_idx = v[i][j];
            ull local_idx = global_idx % CHUNK_SIZE;

            new_amps_1[local_idx].real = amps_1[local_idx].real * cosAngle - amps_1[local_idx].imag * sinAngle;
            new_amps_1[local_idx].imag = amps_1[local_idx].real * sinAngle + amps_1[local_idx].imag * cosAngle;
        }

        // update amps, swapping their address will do
        state.update_amps_1(chunk_id, tid);

        // mark finished
        state.push_chunk_to_stack(chunk_id);
    }
}
#ifndef _STATE_HPP_
#define _STATE_HPP_

#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <string>
#include <queue>

typedef unsigned long long ull;

#define NUM_THREADS 1
#define EXPONENT 21
#define CHUNK_SIZE (1 << EXPONENT)

#define BUFFER_SIZE (10 << (26 - EXPONENT))
#define NOT_IN_BUFFER BUFFER_SIZE
#define NOT_IN_FINISHED BUFFER_SIZE

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
    Complex **amps_buffer;
    Complex **new_amps_buffer1, **new_amps_buffer2;
    ull *chunk_buffer_ids;
    ull *chunk_finished_ids;
    std::vector<ull> finished_stack;

    ull numAmpTotal;
    ull numChunks;
    int numQubitTotal;

    std::string state_file_prefix;
    std::string output_file;

    void createShardedStateFiles();
    void concatShardedStateFiles();

    Complex *get_chunk_by_id(ull chunk_id);
    void read_chunk(ull chunk_id, ull chunk_buffer_id);
    void write_back_chunk(ull chunk_buffer_id);
    void flush_chunk();
    void push_chunk_to_stack(ull chunk_id);
    void remove_chunk_from_stack(ull chunk_id);
    void update_amps_1(ull chunk_id, int tid);
    void update_amps_2(ull chunk_id, int tid);

    // init
    ManagedStateVector(int nqbit, std::string state_file_prefix, std::string output_file);

    // flush chunk, concat state files, free buffer
    void finish_task();
};

#endif

void createState(State &state);
void createShardedStateFiles(State &state, int chunk_size);
void concatShardedStateFiles(const std::string &outputFile, ull numShards, ull chunk_size);
void printState(const State &state, int num);
void destroyState(const State &state);

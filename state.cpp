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
    for (ull i = 1; i < state.numAmpTotal; i++)
    {
        pwrite(fd, &zero, sizeof(Complex), i * sizeof(Complex));
    }
    close(fd);
    return;
}

void createShardedStateFiles(State &state, int chunk_size)
{
    Complex *zero = (Complex *)calloc(chunk_size, sizeof(Complex));
    Complex *one = (Complex *)calloc(chunk_size, sizeof(Complex));
    one[0].real = 1.0f;

    // #pragma omp parallel for num_threads(1)
    for (ull i = 0; i < state.numAmpTotal >> EXPONENT; i++)
    {
        std::ostringstream filename;
        filename << "./shard" << i;

        // file initialization
        int fd = open(filename.str().c_str(), O_RDWR | O_CREAT, 0777);

        // allocate storage
        if (ftruncate(fd, chunk_size * sizeof(Complex)) != 0)
        {
            perror("Failed to allocate space");
            close(fd);
        }

        // init first with one, rest with zero
        if (i == 0)
            pwrite(fd, one, chunk_size * sizeof(Complex), 0);
        else
            pwrite(fd, zero, chunk_size * sizeof(Complex), 0);

        close(fd);
    }
}

void concatShardedStateFiles(const std::string &outputFile, ull numShards, ull chunk_size = 1 << 10)
{
    // Open the output file for writing
    int out_fd = open(outputFile.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_APPEND, 0777);
    if (out_fd < 0)
    {
        perror("Failed to open output file");
        return;
    }

    // if (ftruncate(out_fd, numShards * chunk_size * sizeof(Complex)) != 0)
    // {
    //     perror("Failed to allocate space");
    //     close(out_fd);
    // }

    // Buffer to hold data from each shard
    Complex *buffer = (Complex *)malloc(chunk_size * sizeof(Complex));
    if (!buffer)
    {
        perror("Failed to allocate buffer");
        close(out_fd);
        return;
    }

    for (ull i = 0; i < numShards; i++)
    {
        // Construct shard file name
        std::ostringstream shardFilename;
        shardFilename << "./shard" << i;

        // Open the shard file for reading
        int shard_fd = open(shardFilename.str().c_str(), O_RDONLY);
        if (shard_fd < 0)
        {
            perror(shardFilename.str().c_str());
            free(buffer);
            close(out_fd);
            return;
        }

        // Read the shard file and append its content to the output file
        ssize_t bytesRead = pread(shard_fd, buffer, chunk_size * sizeof(Complex), 0);
        if (bytesRead < 0)
        {
            perror("Failed to read shard file");
            free(buffer);
            close(shard_fd);
            close(out_fd);
            return;
        }

        ssize_t bytesWritten = pwrite(out_fd, buffer, chunk_size * sizeof(Complex), i * chunk_size * sizeof(Complex));
        if (bytesWritten != bytesRead)
        {
            perror("Failed to write to output file");
            free(buffer);
            close(shard_fd);
            close(out_fd);
            return;
        }

        // Close the shard file
        close(shard_fd);
    }

    // Free buffer and close the output file
    free(buffer);
    close(out_fd);
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

ManagedStateVector::ManagedStateVector(int nqbit, std::string state_file_prefix, std::string output_file)
{

    numQubitTotal = nqbit;
    numAmpTotal = 1ull << nqbit;
    numChunks = numAmpTotal >> EXPONENT;
    this->state_file_prefix = state_file_prefix;
    this->output_file = output_file;

    // create state files
    createShardedStateFiles();

    // allocate memory chunk buffer
    amps_buffer = (Complex **)calloc(BUFFER_SIZE, sizeof(Complex *));
    new_amps_buffer1 = (Complex **)calloc(NUM_THREADS, sizeof(Complex *));
    new_amps_buffer2 = (Complex **)calloc(NUM_THREADS, sizeof(Complex *));
    // #pragma omp parallel for num_threads(NUM_THREADS)
    for (ull i = 0; i < BUFFER_SIZE; i++)
    {
        amps_buffer[i] = (Complex *)calloc(CHUNK_SIZE, sizeof(Complex));
    }
    // #pragma omp parallel for num_threads(NUM_THREADS)
    for (int i = 0; i < NUM_THREADS; i++)
    {
        new_amps_buffer1[i] = (Complex *)calloc(CHUNK_SIZE, sizeof(Complex));
        new_amps_buffer2[i] = (Complex *)calloc(CHUNK_SIZE, sizeof(Complex));
    }

    // create id mapping: id in storage -> id in buffer
    chunk_buffer_ids = (ull *)calloc(numChunks, sizeof(ull));
#pragma omp parallel for num_threads(NUM_THREADS)
    for (ull i = 0; i < numChunks; i++)
    {
        chunk_buffer_ids[i] = NOT_IN_BUFFER; // chunk_buffer_ids[i] == NOT_IN_BUFFER -> chunk[i] is in storage
    }

    // create id mapping: id in storage -> id in finished vector (stack)
    chunk_finished_ids = (ull *)calloc(numChunks, sizeof(ull));
#pragma omp parallel for num_threads(NUM_THREADS)
    for (ull i = 0; i < numChunks; i++)
    {
        chunk_finished_ids[i] = NOT_IN_FINISHED;
    }

    // pre-load chunks to memory
    std::cout << "buffer size: " << BUFFER_SIZE << ", num chunks: " << numChunks << std::endl;
    ull limit = (BUFFER_SIZE < numChunks) ? BUFFER_SIZE : numChunks;
#pragma omp parallel for num_threads(NUM_THREADS)
    for (ull i = 0; i < limit; i++)
    {
        read_chunk(i, i);
    }

    // mark the pre-loaded chunks as finished
    // critical
    for (ull i = 0; i < limit; i++)
    {
        push_chunk_to_stack(i);
    }
}

void ManagedStateVector::createShardedStateFiles()
{
    Complex *zero = (Complex *)calloc(CHUNK_SIZE, sizeof(Complex));
    Complex *one = (Complex *)calloc(CHUNK_SIZE, sizeof(Complex));
    one[0].real = 1.0f;

#pragma omp parallel for num_threads(NUM_THREADS)
    for (ull i = 0; i < numAmpTotal >> EXPONENT; i++)
    {
        // file initialization
        std::ostringstream filename;
        filename << state_file_prefix << i;
        int fd = open(filename.str().c_str(), O_RDWR | O_CREAT, 0777);

        // allocate storage
        if (ftruncate(fd, CHUNK_SIZE * sizeof(Complex)) != 0)
        {
            perror("Failed to allocate space");
            close(fd);
        }

        // init first with one, rest with zero
        if (i == 0)
            pwrite(fd, one, CHUNK_SIZE * sizeof(Complex), 0);
        else
            pwrite(fd, zero, CHUNK_SIZE * sizeof(Complex), 0);

        close(fd);
    }
}

void ManagedStateVector::concatShardedStateFiles()
{
    // Open the output file for writing
    int out_fd = open(output_file.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_APPEND, 0777);
    if (out_fd < 0)
    {
        perror("Failed to open output file");
        return;
    }

    // Buffer to hold data from each shard
    Complex *buffer = (Complex *)malloc(CHUNK_SIZE * sizeof(Complex));
    if (!buffer)
    {
        perror("Failed to allocate buffer");
        close(out_fd);
        return;
    }

    // critical
    for (ull i = 0; i < numChunks; i++)
    {
        // Construct shard file name
        std::ostringstream shardFilename;
        shardFilename << state_file_prefix << i;

        // Open the shard file for reading
        int shard_fd = open(shardFilename.str().c_str(), O_RDONLY);
        if (shard_fd < 0)
        {
            perror(shardFilename.str().c_str());
            free(buffer);
            close(out_fd);
            return;
        }

        // Read the shard file and append its content to the output file
        ssize_t bytesRead = pread(shard_fd, buffer, CHUNK_SIZE * sizeof(Complex), 0);
        if (bytesRead < 0)
        {
            perror("Failed to read shard file");
            free(buffer);
            close(shard_fd);
            close(out_fd);
            return;
        }

        ssize_t bytesWritten = pwrite(out_fd, buffer, CHUNK_SIZE * sizeof(Complex), i * CHUNK_SIZE * sizeof(Complex));
        if (bytesWritten != bytesRead)
        {
            perror("Failed to write to output file");
            free(buffer);
            close(shard_fd);
            close(out_fd);
            return;
        }

        // Close the shard file
        close(shard_fd);

        // TODO: remove the shard file after writing
    }

    // Free buffer and close the output file
    free(buffer);
    close(out_fd);
}

void ManagedStateVector::write_back_chunk(ull chunk_id)
{
    ull chunk_buffer_id = chunk_buffer_ids[chunk_id];

    if (chunk_buffer_id == NOT_IN_BUFFER)
    {
        std::cout << "bad! attempt to write a chunk not in memory" << std::endl;
        return;
    }

    // open shard file
    std::ostringstream sharded_filename;
    sharded_filename << state_file_prefix << chunk_id;
    int chunk_fd = open(sharded_filename.str().c_str(), O_RDWR);

    // write chunk to file
    pwrite(chunk_fd, amps_buffer[chunk_buffer_id], CHUNK_SIZE * sizeof(Complex), 0);
    close(chunk_fd);

    // update id mapping
    chunk_buffer_ids[chunk_id] = NOT_IN_BUFFER;
}

void ManagedStateVector::read_chunk(ull chunk_id, ull chunk_buffer_id)
{
    if (chunk_buffer_ids[chunk_id] != NOT_IN_BUFFER)
    {
        std::cout << "bad! attempt to load a chunk already in memory" << std::endl;
        return;
    }

    // open shard file
    std::ostringstream sharded_filename;
    sharded_filename << state_file_prefix << chunk_id;
    int chunk_fd = open(sharded_filename.str().c_str(), O_RDWR);

    // read chunk from file
    pread(chunk_fd, amps_buffer[chunk_buffer_id], CHUNK_SIZE * sizeof(Complex), 0);
    close(chunk_fd);

    // update id mapping
    chunk_buffer_ids[chunk_id] = chunk_buffer_id;
}

Complex *ManagedStateVector::get_chunk_by_id(ull chunk_id)
{
    // chunk not present in memory, load from storage
    if (chunk_buffer_ids[chunk_id] == NOT_IN_BUFFER)
    {
        // select a memory chunk for swapping
        ull swapped_chunk_id, swapped_chunk_buffer_id;
#pragma omp critical(stack)
        {
            swapped_chunk_id = finished_stack.back();
            swapped_chunk_buffer_id = chunk_buffer_ids[swapped_chunk_id];

            // update finished stack
            finished_stack.pop_back();
            chunk_finished_ids[swapped_chunk_id] = NOT_IN_FINISHED;
        }

        // swap the two chunks
        write_back_chunk(swapped_chunk_id);
        read_chunk(chunk_id, swapped_chunk_buffer_id);
    }
    else
#pragma omp critical(stack)
    {
        // std::cout << chunk_id << " hit!" << std::endl;
        // chunk in finished stack, remove it from finished stack
        if (chunk_finished_ids[chunk_id] != NOT_IN_FINISHED)
        {
            remove_chunk_from_stack(chunk_id);
        }
    }

    return amps_buffer[chunk_buffer_ids[chunk_id]];
}

void ManagedStateVector::flush_chunk()
{
    // write back all chunks in memory to storage
#pragma omp parallel for num_threads(NUM_THREADS)
    for (ull i = 0; i < numChunks; i++)
    {
        if (chunk_buffer_ids[i] != NOT_IN_BUFFER)
        {
            write_back_chunk(i);
        }
    }
}

void ManagedStateVector::push_chunk_to_stack(ull chunk_id)
{
    // push chunk to finished stack
#pragma omp critical(stack)
    {
        finished_stack.push_back(chunk_id);
        chunk_finished_ids[chunk_id] = finished_stack.size() - 1;
    }
}

void ManagedStateVector::remove_chunk_from_stack(ull chunk_id)
{
    // remove chunk from finished stack
    {
        chunk_finished_ids[chunk_id] = NOT_IN_FINISHED;
        for (ull i = chunk_finished_ids[chunk_id] + 1; i < finished_stack.size(); i++)
        {
            chunk_finished_ids[finished_stack[i]] -= 1;
        }

        finished_stack.erase(finished_stack.begin() + chunk_finished_ids[chunk_id]);
    }
}

void ManagedStateVector::update_amps_1(ull chunk_id, int tid)
{
    Complex *temp = amps_buffer[chunk_buffer_ids[chunk_id]];
    amps_buffer[chunk_buffer_ids[chunk_id]] = new_amps_buffer1[tid];
    new_amps_buffer1[tid] = temp;
}

void ManagedStateVector::update_amps_2(ull chunk_id, int tid)
{
    Complex *temp = amps_buffer[chunk_buffer_ids[chunk_id]];
    amps_buffer[chunk_buffer_ids[chunk_id]] = new_amps_buffer2[tid];
    new_amps_buffer2[tid] = temp;
}

void ManagedStateVector::finish_task()
{
    // write back all chunk
    flush_chunk();
    concatShardedStateFiles();

    // free up space
#pragma omp parallel for num_threads(NUM_THREADS)
    for (ull i = 0; i < BUFFER_SIZE; i++)
    {
        free(amps_buffer[i]);
    }
#pragma omp parallel for num_threads(NUM_THREADS)
    for (ull i = 0; i < NUM_THREADS; i++)
    {
        free(new_amps_buffer1[i]);
        free(new_amps_buffer2[i]);
    }

    free(amps_buffer);
    free(new_amps_buffer1);
    free(new_amps_buffer2);
    free(chunk_buffer_ids);
}
#include <mpi.h>

#include <iostream>
#include <vector>
#include <cstdlib>
#include <cstdint>
#include <numeric>

#include "glue.hpp"
#include "mpiutil.hpp"

void work(int global_rank, int global_size, int local_rank,
        float min_delay, float run_time)
{
    const int sbuf_length = 0;
    std::vector<int> rbuf_lengths(global_size);
    std::vector<proxy_spike> rbuf;
    const std::vector<int> sbuf(1);

    for (; run_time > 0; run_time -= min_delay) {
        if (local_rank == 0) {
            std::cerr << "Time left: " << run_time << std::endl;
        }

        MPI_Allgather(&sbuf_length, 1, MPI_INT,
                &rbuf_lengths[0], 1, MPI_INT,
                MPI_COMM_WORLD);

        auto total_chars = std::accumulate(rbuf_lengths.begin(),
                rbuf_lengths.end(),
                0);
        std::vector< int > rbuf_offset;
        rbuf_offset.reserve(global_size);
        for (int i = 0, c_offset = 0; i < global_size; i++) {
            rbuf_offset.push_back(c_offset);
            c_offset += rbuf_lengths[i];
        }

        // spikes
        rbuf.resize(total_chars/sizeof(proxy_spike));
        MPI_Allgatherv(
                &sbuf[0], sbuf_length, MPI_CHAR,
                &rbuf[0], &rbuf_lengths[0], &rbuf_offset[0], MPI_CHAR,
                MPI_COMM_WORLD);

        if (local_rank == 0) {
            for (auto spike: rbuf) {
                auto gid = spike.gid;
                auto time = spike.time;

                std::cerr << "Gid: " << gid << ", Time: " << time << std::endl;
            }
        }
    }
}

// argv[1] = min delay
// argv[2] = run time
int main(int argc, char **argv)
{
    float min_delay = std::stof(argv[1]);
    float run_time = std::stof(argv[2]);

    int rank, size;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    /* Build intra-communicator for local sub-group */
    MPI_Comm   intracomm;  /* intra-communicator of local sub-group */ 
    MPI_Comm_split(MPI_COMM_WORLD, 0, rank, &intracomm);

    int lrank, lsize;
    MPI_Comm_rank(intracomm, &lrank);
    MPI_Comm_size(intracomm, &lsize);
    if (lrank == 0) {
        std::cerr << "Starting arbor_proxy: size=" << size << ", min_delay=" << min_delay << ", run_time=" << run_time << std::endl;
        std::cerr << "Local size: " << lsize << std::endl;
    }

    // work
    work(rank, size, lrank, min_delay, run_time);

    // cleanup
    MPI_Comm_free(&intracomm);
    MPI_Finalize();

    return 0;
}


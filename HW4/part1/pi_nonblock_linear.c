#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#pragma GCC optimize ("O3")

long long int local_hit = 0;

void estimate_pi(long long int tosses) {
    unsigned int seed = (unsigned int)time(NULL) ^ (unsigned int)getpid(); 
    for (long long toss = 0; toss < tosses; ++toss) {
        float x = ((float)rand_r(&seed) / RAND_MAX) * 2 - 1;
        float y = ((float)rand_r(&seed) / RAND_MAX) * 2 - 1; 
        float distance_squared = x * x + y * y;

        if (distance_squared <= 1.0)
            local_hit++;
    }
}

int main(int argc, char **argv)
{
    // --- DON'T TOUCH ---
    MPI_Init(&argc, &argv);
    double start_time = MPI_Wtime();
    double pi_result;
    long long int tosses = atoi(argv[1]);
    int world_rank, world_size;
    // ---

    // TODO: MPI init
    const int target_rank = 0;
    const int message_tag = 99;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    long long int local_tosses = (tosses + (world_size-1)) / world_size;
    long long int hit = 0;



    if (world_rank > 0)
    {
        // TODO: MPI workers
        estimate_pi(local_tosses);
        MPI_Send(&local_hit,
                 1,
                 MPI_LONG_LONG_INT,
                 target_rank,
                 message_tag,
                 MPI_COMM_WORLD);
    }
    else if (world_rank == 0)
    {
        // TODO: non-blocking MPI communication.
        // Use MPI_Irecv, MPI_Wait or MPI_Waitall.

        // MPI_Request requests[];
        MPI_Request requests[world_size];
        long long int buffer[world_size];

        for (size_t i = 1; i < world_size; ++i)
        {
            MPI_Irecv(&buffer[i],
                      1,
                      MPI_LONG_LONG_INT,
                      i,
                      message_tag,
                      MPI_COMM_WORLD,
                      &requests[i]);
        }

        estimate_pi(local_tosses);
        hit = local_hit;

        // MPI_Waitall();
        MPI_Waitall(world_size-1,
                    &requests[1], // request[0]用不到 所以array從1開始
                    MPI_STATUS_IGNORE);

        for (size_t i = 1; i < world_size; ++i)
        {
            hit += buffer[i];
        }
    }

    if (world_rank == 0)
    {
        // TODO: PI result
        pi_result = 4.0 * hit / (double)tosses;

        // --- DON'T TOUCH ---
        double end_time = MPI_Wtime();
        printf("%lf\n", pi_result);
        printf("MPI running time: %lf Seconds\n", end_time - start_time);
        // ---
    }

    MPI_Finalize();
    return 0;
}

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

    MPI_Win win;

    // TODO: MPI init
    const int target_rank = 0;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    long long int local_tosses = (tosses + (world_size-1)) / world_size;
    long long int hit = 0;

    estimate_pi(local_tosses);

    if (world_rank == 0)
    {
        // Main
        MPI_Win_create(&hit,
                       sizeof(long long int),
                       sizeof(long long int),
                       MPI_INFO_NULL,
                       MPI_COMM_WORLD,
                       &win);
        hit += local_hit;
    }
    else
    {
        // Workers
        MPI_Win_create(NULL,
                       0,
                       sizeof(long long int),
                       MPI_INFO_NULL,
                       MPI_COMM_WORLD,
                       &win);

        MPI_Win_lock(MPI_LOCK_EXCLUSIVE, target_rank, 0, win);
        MPI_Accumulate(&local_hit,
                       1,
                       MPI_LONG_LONG_INT,
                       target_rank,
                       0,
                       1,
                       MPI_LONG_LONG_INT,
                       MPI_SUM,
                       win);
        MPI_Win_unlock(target_rank, win);
    }

    MPI_Win_free(&win);
    MPI_Barrier(MPI_COMM_WORLD);

    if (world_rank == 0)
    {
        // TODO: handle PI result
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

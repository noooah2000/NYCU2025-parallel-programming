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
    const int message_tag = 99;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    long long int local_tosses = (tosses + (world_size-1)) / world_size;

    estimate_pi(local_tosses);
    long long int hit = local_hit;

    // TODO: binary tree redunction
    size_t chack_bit = 1;
    size_t remain_pairs = world_size >> 1; // 還有幾對

    while (remain_pairs)
    {
        size_t partner = world_rank ^ chack_bit;
        if (world_rank & chack_bit)
        {
            MPI_Send(&hit,
                     1,
                     MPI_LONG_LONG_INT,
                     partner,
                     message_tag,
                     MPI_COMM_WORLD);
            break;
        }
        else 
        {
            long long int buffer = 0;
            MPI_Recv(&buffer,
                     1,
                     MPI_LONG_LONG_INT,
                     partner,
                     message_tag,
                     MPI_COMM_WORLD,
                     MPI_STATUS_IGNORE);
            hit += buffer;
        }
        chack_bit <<= 1;
        remain_pairs >>= 1;
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

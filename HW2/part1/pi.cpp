#include <iostream>
#include <pthread.h>
#include <string>
#include "prng.h"

long long number_in_circle = 0;
pthread_mutex_t mutex_sum;

typedef struct {
    long long start_idx;
    long long end_idx;
} Arg;


void* Count(void* args) {
    Arg* thread_args = static_cast<Arg*>(args);
    long long start_idx = thread_args->start_idx;
    long long end_idx   = thread_args->end_idx;

    long long local_count = 0;

    __m256i sx = make_seed_vec(static_cast<uint32_t>(end_idx ^ 0x9E3779B9u));
    __m256i sy = make_seed_vec(static_cast<uint32_t>(end_idx ^ 0x85EBCA6Bu));

    const __m256 one = _mm256_set1_ps(1.0f);
    long long toss = start_idx;
    for (; toss + 8 <= end_idx; toss += 8) {
        sx = xs32_vec(sx);
        sy = xs32_vec(sy);

        __m256 x = u31_to_neg1_pos1_vec(sx);
        __m256 y = u31_to_neg1_pos1_vec(sy);

        __m256 xx = _mm256_mul_ps(x, x);
        __m256 yy = _mm256_mul_ps(y, y);
        __m256 dist2 = _mm256_add_ps(xx, yy);

        __m256 cmp = _mm256_cmp_ps(dist2, one, _CMP_LE_OQ);
        int mask = _mm256_movemask_ps(cmp);
        local_count += __builtin_popcount(mask);
    }

    uint32_t sx_s = static_cast<uint32_t>(end_idx ^ 0xDEADBEEFu);
    uint32_t sy_s = static_cast<uint32_t>(end_idx ^ 0xC001D00Du);
    if (sx_s == 0) sx_s = 1;
    if (sy_s == 0) sy_s = 1;
    for (; toss < end_idx; toss++) {
        float x = u31_to_neg1_pos1(xs32(sx_s));
        float y = u31_to_neg1_pos1(xs32(sy_s));
        float d2 = x * x + y * y;
        if (d2 <= 1.0f) local_count++;
    }

    pthread_mutex_lock(&mutex_sum);
    number_in_circle += local_count;
    pthread_mutex_unlock(&mutex_sum);

    pthread_exit(nullptr);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <number_of_threads> <number_of_tosses>" << std::endl;
        return 1; 
    }

    const int number_threads = std::stoi(argv[1]);
    const long long number_tosses = std::stoll(argv[2]);

    pthread_mutex_init(&mutex_sum, nullptr);

    pthread_t* thread = new pthread_t[number_threads];
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    long long tosses_per_thread = number_tosses / number_threads;
    long long remaining_tosses  = number_tosses % number_threads;

    Arg* args = new Arg[number_threads];
    for (int i = 0; i < number_threads; i++) {
        args[i].start_idx = tosses_per_thread * i;
        args[i].end_idx   = tosses_per_thread * (i + 1);
        if (i == number_threads - 1) {
            args[i].end_idx += remaining_tosses;
        }
        pthread_create(&thread[i], &attr, Count, static_cast<void*>(&args[i]));
    }
    pthread_attr_destroy(&attr);

    for (int i = 0; i < number_threads; i++) {
        pthread_join(thread[i], nullptr);
    }

    delete[] thread;
    delete[] args;

    pthread_mutex_destroy(&mutex_sum);

    double pi_estimate = 4.0 * number_in_circle / static_cast<double>(number_tosses);
    std::cout.precision(6);
    std::cout << pi_estimate << std::endl;

    return 0;
}

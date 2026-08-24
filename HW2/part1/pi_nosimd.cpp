#include <iostream>
#include <random>
#include <cmath>
#include <cstdlib>
#include <pthread.h>
#include <string>

long long number_in_circle = 0; 
pthread_mutex_t mutex_sum; 

typedef struct {
    long long start_idx;
    long long end_idx;
} Arg;

void* Count(void* args) {
    Arg* thread_args = static_cast<Arg*>(args);
    long long start_idx = thread_args->start_idx; 
    long long end_idx = thread_args->end_idx;     

    long long local_count = 0; 


    std::mt19937 gen(static_cast<long long>(end_idx));
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f); 

    for (long long toss = start_idx; toss < end_idx; toss++) {
        float x = dist(gen); 
        float y = dist(gen); 
        float distance_squared = x * x + y * y; 

        if (distance_squared <= 1.0f) 
            local_count++;
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
    long long remaining_tosses = number_tosses % number_threads; 
    Arg* args = new Arg[number_threads];
    for (int i = 0; i < number_threads; i++) {
        args[i].start_idx = tosses_per_thread * i;
        args[i].end_idx = tosses_per_thread * (i + 1);

        if (i == number_threads-1) {
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
    
    // 最終結果使用 double 確保最高的輸出精度
    double pi_estimate = 4.0 * number_in_circle / static_cast<double>(number_tosses);
    std::cout << pi_estimate << std::endl;

    return 0;
}
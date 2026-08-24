#include <array>
#include <cstdio>
#include <cstdlib>
#include <thread>
#include <emmintrin.h> 

struct WorkerArgs
{
    float x0, x1;
    float y0, y1;
    unsigned int width;
    unsigned int height;
    int maxIterations;
    int *output;
    int threadId;
    int numThreads;
};

static inline int mandel(float c_re, float c_im, int count)
{
    float z_re = c_re, z_im = c_im;
    int i;
    for (i = 0; i < count; ++i)
    {

        if (z_re * z_re + z_im * z_im > 4.f)
            break;

        float new_re = (z_re * z_re) - (z_im * z_im);
        float new_im = 2.f * z_re * z_im;
        z_re = c_re + new_re;
        z_im = c_im + new_im;
    }

    return i;
}

static void mandelbrot_serial(float x0,
                              float y0,
                              float x1,
                              float y1,
                              int width,
                              int height,
                              int start_row,
                              int num_rows,
                              int max_iterations,
                              int *output)
{
    float dx = (x1 - x0) / (float)width;
    float dy = (y1 - y0) / (float)height;
    int end_row = start_row + num_rows;
    int VEC = 4;
    __m128 four_ps = _mm_set1_ps(4.0f);
    __m128 two_ps  = _mm_set1_ps(2.0f);
    __m128 x0_ps   = _mm_set1_ps(x0);
    __m128 dx_ps   = _mm_set1_ps(dx);

    for (int i = start_row; i < end_row; i++)
    {
        float y = y0 + ((float)i * dy);
        __m128 c_im = _mm_set1_ps(y);

        int j = 0;
        for (; j + VEC - 1 < width; j += VEC)
        {
            __m128 idx_ps = _mm_setr_ps((float)j, (float)(j+1),
                                        (float)(j+2), (float)(j+3));
            __m128 c_re = _mm_add_ps(_mm_mul_ps(idx_ps, dx_ps), x0_ps);
            __m128 z_re = c_re;
            __m128 z_im = c_im;
            __m128i iters = _mm_setzero_si128();

            for (int it = 0; it < max_iterations; it++)
            {
                __m128 zr2  = _mm_mul_ps(z_re, z_re);
                __m128 zi2  = _mm_mul_ps(z_im, z_im);
                __m128 mag2 = _mm_add_ps(zr2, zi2);

                __m128 not_diverged = _mm_cmple_ps(mag2, four_ps);
                if (_mm_movemask_ps(not_diverged) == 0) break;

                __m128i maski = _mm_castps_si128(not_diverged);
                __m128i ones  = _mm_set1_epi32(1);
                iters = _mm_add_epi32(iters, _mm_and_si128(maski, ones));

                __m128 new_re = _mm_add_ps(_mm_sub_ps(zr2, zi2), c_re);
                __m128 two_zrzi = _mm_mul_ps(_mm_mul_ps(two_ps, z_re), z_im);
                __m128 new_im   = _mm_add_ps(two_zrzi, c_im);

                z_re = _mm_or_ps(_mm_and_ps(not_diverged, new_re),
                                 _mm_andnot_ps(not_diverged, z_re));
                z_im = _mm_or_ps(_mm_and_ps(not_diverged, new_im),
                                 _mm_andnot_ps(not_diverged, z_im));
            }
            alignas(16) int tmp[VEC];
            _mm_storeu_si128((__m128i*)tmp, iters);
            int base = i * width + j;
            output[base + 0] = tmp[0];
            output[base + 1] = tmp[1];
            output[base + 2] = tmp[2];
            output[base + 3] = tmp[3];
        }

        for (; j < width; j++)
        {
            float x = x0 + ((float)j * dx);
            int idx = i * width + j;
            output[idx] = mandel(x, y, max_iterations);
        }
    }
}

//
// worker_thread_start --
//
// Thread entrypoint.
void worker_thread_start(WorkerArgs *const args)
{
    int thread_id = args->threadId;
    int num_threads = args->numThreads;
    int width = args->width;
    int height = args->height;

    for (int i = thread_id; i < height; i += num_threads) {
        mandelbrot_serial(args->x0, args->y0, args->x1, args->y1,
                          width, height, i, 1, args->maxIterations, args->output);
    }

}

//
// mandelbrot_thread --
//
// Multi-threaded implementation of mandelbrot set image generation.
// Threads of execution are created by spawning std::threads.
void mandelbrot_thread(int num_threads,
                       float x0,
                       float y0,
                       float x1,
                       float y1,
                       int width,
                       int height,
                       int max_iterations,
                       int *output)
{
    static constexpr int max_threads = 32;

    if (num_threads > max_threads)
    {
        fprintf(stderr, "Error: Max allowed threads is %d\n", max_threads);
        exit(1);
    }

    // Creates thread objects that do not yet represent a thread.
    std::array<std::thread, max_threads> workers;
    std::array<WorkerArgs, max_threads> args = {};

    for (int i = 0; i < num_threads; i++)
    {
        // TODO FOR PP STUDENTS: You may or may not wish to modify
        // the per-thread arguments here.  The code below copies the
        // same arguments for each thread
        args[i].x0 = x0;
        args[i].y0 = y0;
        args[i].x1 = x1;
        args[i].y1 = y1;
        args[i].width = width;
        args[i].height = height;
        args[i].maxIterations = max_iterations;
        args[i].numThreads = num_threads;
        args[i].output = output;

        args[i].threadId = i;
    }

    // Spawn the worker threads.  Note that only numThreads-1 std::threads
    // are created and the main application thread is used as a worker
    // as well.
    for (int i = 1; i < num_threads; i++)
    {
        workers[i] = std::thread(worker_thread_start, &args[i]);
    }

    worker_thread_start(&args[0]);

    // join worker threads
    for (int i = 1; i < num_threads; i++)
    {
        workers[i].join();
    }
}

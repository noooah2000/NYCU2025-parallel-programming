#include <cstdio>
#include <cstdlib>
#include <cuda.h>
#include <cstring> 
#include "kernel.h"

#define BLOCK_DIM_X 16
#define BLOCK_DIM_Y 16
#define GRID_DIM_X 100
#define GRID_DIM_Y 75
#define GROUP_SIZE 2

__device__ inline int mandel_gpu(float c_re, float c_im, int max)
{
    float z_re = c_re, z_im = c_im;
    int i;
    for (i = 0; i < max; ++i)
    {

        if (z_re * z_re + z_im * z_im > 4.f) break;
        float new_re = z_re * z_re - z_im * z_im;
        float new_im = 2.f * z_re * z_im;
        z_re = c_re + new_re;
        z_im = c_im + new_im;
    }

    return i;
}

__global__ void mandel_kernel(int *d_buffer,
                              size_t pitch_elements,
                              float lower_x, 
                              float lower_y,
                              float step_x, 
                              float step_y,
                              int max_iters)
{
    // To avoid error caused by the floating number, use the following pseudo code
    //
    // float x = lowerX + thisX * stepX;
    // float y = lowerY + thisY * stepY;

    int pixel_group_idx_x = (blockIdx.x * blockDim.x + threadIdx.x) * GROUP_SIZE;  // x 方向 pixel group index
    int pixel_idx_y = blockIdx.y * blockDim.y + threadIdx.y;  // y 方向 pixel index

    float y = lower_y + pixel_idx_y * step_y;
    int row_offset = pixel_idx_y * pitch_elements; 

    #pragma unroll
    for (int i = 0; i < GROUP_SIZE; ++i)
    {
        float x = lower_x + (pixel_group_idx_x + i) * step_x;

        int idx = row_offset + (pixel_group_idx_x + i);
        d_buffer[idx] = mandel_gpu(x, y, max_iters);
    }   
}

// Host front-end function that allocates the memory and launches the GPU kernel
void host_fe(float upper_x, // x1
             float upper_y, // y1
             float lower_x, // x0
             float lower_y, // y0
             int *img, // output
             int res_x, // Resolution of x = width
             int res_y, // Resolution of y = height
             int max_iterations)
{
    float step_x = (upper_x - lower_x) / (float)res_x;
    float step_y = (upper_y - lower_y) / (float)res_y;

    size_t num_pixels = (size_t)res_x * (size_t)res_y;
    size_t total_size_bytes = num_pixels * sizeof(int);
    size_t row_size_bytes = (size_t)res_x * sizeof(int);

    int *h_buffer = nullptr;
    cudaHostAlloc(&h_buffer, total_size_bytes, cudaHostAllocDefault);

    int *d_buffer = nullptr;
    size_t pitch;
    cudaMallocPitch(&d_buffer, &pitch, row_size_bytes, res_y);

    dim3 block_dim(BLOCK_DIM_X, BLOCK_DIM_Y);
    dim3 grid_dim((GRID_DIM_X / GROUP_SIZE), GRID_DIM_Y);

    size_t pitch_in_ints = pitch / sizeof(int);
    mandel_kernel<<<grid_dim, block_dim>>>(d_buffer,
                                           pitch_in_ints,
                                           lower_x, 
                                           lower_y,
                                           step_x, 
                                           step_y,
                                           max_iterations);

    cudaDeviceSynchronize();
    cudaMemcpy2D(h_buffer, row_size_bytes, d_buffer, pitch, row_size_bytes, res_y, cudaMemcpyDeviceToHost);
    std::memcpy(img, h_buffer, total_size_bytes);
    cudaFreeHost(h_buffer);
    cudaFree(d_buffer);
}

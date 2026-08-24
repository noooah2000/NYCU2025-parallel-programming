#include <cstdio>
#include <cstdlib>
#include <cuda.h>
#include <cstring> 
#include "kernel.h"

#define BLOCK_DIM_X 16
#define BLOCK_DIM_Y 16
#define GRID_DIM_X 100
#define GRID_DIM_Y 75

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
                              float lower_x, 
                              float lower_y,
                              float step_x, 
                              float step_y,
                              int res_x,
                              int max_iters)
{
    // To avoid error caused by the floating number, use the following pseudo code
    //
    // float x = lowerX + thisX * stepX;
    // float y = lowerY + thisY * stepY;

    int pixel_idx_x = blockIdx.x * blockDim.x + threadIdx.x;  // x 方向 pixel index
    int pixel_idx_y = blockIdx.y * blockDim.y + threadIdx.y;  // y 方向 pixel index

    float x = lower_x + pixel_idx_x * step_x;
    float y = lower_y + pixel_idx_y * step_y;

    int idx = pixel_idx_y * res_x + pixel_idx_x;
    d_buffer[idx] = mandel_gpu(x, y, max_iters);
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

    int *h_buffer = new int[num_pixels];

    int *d_buffer = nullptr;
    cudaMalloc(&d_buffer, total_size_bytes);

    dim3 block_dim(BLOCK_DIM_X, BLOCK_DIM_Y);
    dim3 grid_dim(GRID_DIM_X, GRID_DIM_Y);
    mandel_kernel<<<grid_dim, block_dim>>>(d_buffer,
                                           lower_x, 
                                           lower_y,
                                           step_x, 
                                           step_y,
                                           res_x,
                                           max_iterations);

    cudaDeviceSynchronize();
    cudaMemcpy(h_buffer, d_buffer, total_size_bytes, cudaMemcpyDeviceToHost);
    std::memcpy(img, h_buffer, total_size_bytes);
    delete[] h_buffer;
    cudaFree(d_buffer);
}



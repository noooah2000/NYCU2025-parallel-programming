#include "host_fe.h"
#include "helper.h"
#include <stdio.h>
#include <stdlib.h>

void host_fe(int filter_width,
             float *filter,
             int image_height,
             int image_width,
             float *input_image,
             float *output_image,
             cl_device_id *device,
             cl_context *context,
             cl_program *program)
{
    cl_int status;

    int eff_width = filter_width;
    int top = 0, bottom = filter_width - 1;
    int left = 0, right = filter_width - 1;

    while (eff_width > 1) 
    {
        int has_nonzero = 0;

        for (int x = left; x <= right && !has_nonzero; ++x) 
        {
            float top_val = filter[top * filter_width + x];
            float bottom_val = filter[bottom * filter_width + x];
            if (top_val != 0.0f || bottom_val != 0.0f) 
            {
                has_nonzero = 1;
                break;
            }
        }

        for (int y = top; y <= bottom && !has_nonzero; ++y) 
        {
            float left_val  = filter[y * filter_width + left];
            float right_val = filter[y * filter_width + right];
            if (left_val != 0.0f || right_val != 0.0f) 
            {
                has_nonzero = 1;
                break;
            }
        }

        if (has_nonzero) break;

        ++top;
        --bottom;
        ++left;
        --right;
        eff_width -= 2;
    }

    size_t filter_size_bytes = (size_t)eff_width * eff_width * sizeof(float);
    size_t data_size_bytes = (size_t)image_height * image_width * sizeof(float);

    float *eff_filter = filter;
    if (eff_width != filter_width) 
    {
        eff_filter = (float *)malloc(filter_size_bytes);

        int idx = 0;
        for (int y = top; y <= bottom; ++y) 
        {
            for (int x = left; x <= right; ++x) 
            {
                eff_filter[idx++] = filter[y * filter_width + x];
            }
        }
    }

    static cl_command_queue command_queue = NULL;
    static cl_kernel kernel_func = NULL;
    static cl_mem output_buf = NULL;
    static size_t cached_data_size_bytes = 0;
    static size_t cached_eff_width = 0;

    cl_mem filter_buf = clCreateBuffer(*context, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR, 
                                       filter_size_bytes, eff_filter, &status);
    CHECK(status, "clCreateBuffer");

    cl_mem input_buf = clCreateBuffer(*context, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR, 
                                      data_size_bytes, input_image, &status);
    CHECK(status, "clCreateBuffer");

    if (output_buf == NULL || cached_data_size_bytes != data_size_bytes)
    {
        if (output_buf) clReleaseMemObject(output_buf);
        output_buf = clCreateBuffer(*context, CL_MEM_WRITE_ONLY, 
                                    data_size_bytes, NULL, &status);
        CHECK(status, "clCreateBuffer");
        cached_data_size_bytes = data_size_bytes;
    }

    if (command_queue == NULL) 
    {
        command_queue = clCreateCommandQueue(*context, *device, 0, &status);
        CHECK(status, "clCreateCommandQueue");
    }

    if (kernel_func == NULL || cached_eff_width != eff_width)
    {
        if (kernel_func) clReleaseKernel(kernel_func);
        if (eff_width == 3) 
        {
            kernel_func = clCreateKernel(*program, "convolution3x3", &status);
        }
        else
        {
            kernel_func = clCreateKernel(*program, "convolution", &status);
        }
        CHECK(status, "clCreateKernel");
        cached_eff_width = eff_width;
    }

    status = clSetKernelArg(kernel_func, 0, sizeof(cl_mem), (void *)&filter_buf);
    CHECK(status, "clSetKernelArg");
    status = clSetKernelArg(kernel_func, 1, sizeof(cl_mem), (void *)&input_buf);
    CHECK(status, "clSetKernelArg");
    status = clSetKernelArg(kernel_func, 2, sizeof(cl_mem), (void *)&output_buf);
    CHECK(status, "clSetKernelArg");
    status = clSetKernelArg(kernel_func, 3, sizeof(int), (void *)&eff_width);
    CHECK(status, "clSetKernelArg");
    status = clSetKernelArg(kernel_func, 4, sizeof(int), (void *)&image_height);
    CHECK(status, "clSetKernelArg");
    status = clSetKernelArg(kernel_func, 5, sizeof(int), (void *)&image_width);
    CHECK(status, "clSetKernelArg");


    size_t local_work_size[2] = {32, 16};
    size_t global_work_size[2];
    global_work_size[0] = ((image_width + 31) >> 5) << 5;
    global_work_size[1] = ((image_height + 15) >> 4) << 4;

    status = clEnqueueNDRangeKernel(command_queue, kernel_func, 2, NULL, 
                                    global_work_size, local_work_size,
                                    0, NULL, NULL);
    CHECK(status, "clEnqueueNDRangeKernel");

    status = clEnqueueReadBuffer(command_queue, output_buf, CL_TRUE, 0, 
                                 data_size_bytes, output_image, 
                                 0, NULL, NULL);
    CHECK(status, "clEnqueueReadBuffer");

    if (eff_filter != filter) free(eff_filter);
}
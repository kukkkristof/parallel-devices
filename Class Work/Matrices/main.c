#include "kernel_loader.h"

#define CL_TARGET_OPENCL_VERSION 220
#include <CL/cl.h>

#include <stdio.h>
#include <stdlib.h>

#define TILE_SIZE 2

void print_matrix(float* matrix, int rows, int cols);

int main(void)
{
    cl_platform_id platform;
    cl_device_id device;
    cl_context context;
    cl_command_queue queue;
    cl_kernel kernel;
    cl_int err;

    // Select the first available platform and GPU device
    clGetPlatformIDs(1, &platform, NULL);
    clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);

    // Create an OpenCL context and command queue
    context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    queue = clCreateCommandQueue(context, device, 0, &err);

    // Create and build the program
    const char* kernel_code = load_kernel_source("kernels/matrices.cl", &err);
    if (err != 0) {
        printf("Source code loading error!\n");
        return 0;
    }
    cl_program program = clCreateProgramWithSource(context, 1, &kernel_code, NULL, &err);

    printf("clCreateProgramWithSource: %d\n", err);

    err = clBuildProgram(
        program,
        1,
        &device,
        NULL,
        NULL,
        NULL
    );

    printf("clBuildProgram: %d\n", err);

    // Create the kernel
    kernel = clCreateKernel(program, "transpose", &err);

    printf("clCreateKernel: %d\n", err);

    // Matrix dimensions
    int dim_n = 2;
    int dim_m = 4;

    // Allocate and initialize host memory
    float* matrix = (float*)malloc(dim_n * dim_m * sizeof(float));
    float* transposed_matrix = (float*)malloc(dim_n * dim_m * sizeof(float));
    for (int i = 0; i < dim_n * dim_m; i++) {
        matrix[i] = i+1;
    }

    // Create device buffers
    cl_mem d_matrix = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,
                                     dim_n * dim_m * sizeof(float), matrix, &err);
    cl_mem d_transposed = clCreateBuffer(context, CL_MEM_WRITE_ONLY,
                                         dim_n * dim_m * sizeof(float), NULL, &err);

    // Set kernel arguments
    clSetKernelArg(kernel, 0, sizeof(cl_mem), &d_matrix);
    clSetKernelArg(kernel, 1, sizeof(int), &dim_n);
    clSetKernelArg(kernel, 2, sizeof(int), &dim_m);
    clSetKernelArg(kernel, 3, sizeof(cl_mem), &d_transposed);

    // Set up global and local work sizes
    size_t global_work_size[2] = {dim_m, dim_n};
    size_t local_work_size[2] = {TILE_SIZE, TILE_SIZE};


    // Enqueue kernel
    clEnqueueNDRangeKernel(queue, kernel, 2, NULL, global_work_size, local_work_size, 0, NULL, NULL);
    clFinish(queue);
    // Read back the result
    clEnqueueReadBuffer(queue, d_transposed, CL_TRUE, 0,
                        dim_n * dim_m * sizeof(float), transposed_matrix, 0, NULL, NULL);

    print_matrix(matrix, dim_n, dim_m);
    print_matrix(transposed_matrix, dim_m, dim_n);

    // Clean up
    clReleaseMemObject(d_matrix);
    clReleaseMemObject(d_transposed);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);



    free(matrix);
    free(transposed_matrix);

    return 0;
}

void print_matrix(float* matrix, int rows, int cols)
{
  for(int i = 0; i < rows * cols; i++)
  {
    printf("%.0f ", matrix[i]);
    if((i+1)%cols == 0)
    {
      printf("\n");
    }
  }
}
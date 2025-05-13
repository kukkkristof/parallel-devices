#include "kernel_loader.h"
#include <stdio.h>
#include <BMPFILE.h>
#include <pthread.h>
#include <CL/cl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>


#include <unistd.h>

#define CL_TARGET_OPENCL_VERSION 220
#define N_THREADS 16

typedef struct Argument_t
{
    RGB *rgb_array;
    uint32_t width;
    uint32_t height;
    uint8_t ascending;
    uint32_t start;
    uint32_t end;
} Argument_t;

void sort_horizontal(RGB* rgb_array, uint32_t width, uint32_t height, uint8_t ascending, uint32_t start_y, uint32_t end_y);
void sort_vertical(RGB* rgb_array, uint32_t width, uint32_t height, uint8_t ascending, uint32_t start_x, uint32_t end_x);
void swap_rgb(RGB* A, RGB* B);

void *call_parallel_sort_horizontal(void *arg)
{
    Argument_t *argument = (Argument_t*)arg;
    sort_horizontal(argument->rgb_array, argument->width, argument->height, argument->ascending, argument->start, argument->end);
    return NULL;
}

void *call_parallel_sort_vertical(void *arg)
{
    Argument_t *argument = (Argument_t*)arg;
    sort_vertical(argument->rgb_array, argument->width, argument->height, argument->ascending, argument->start, argument->end);
    return NULL;
}

double elapsed_ms(struct timeval *start, struct timeval *end) {
    return (end->tv_sec - start->tv_sec) * 1000.0 +
           (end->tv_usec - start->tv_usec) / 1000.0;
}

int main(int argc, char *argv[])
{
    // -----------------------------
    // Parse local_work_size argument
    // -----------------------------
    size_t local_work_size = 128;
    if (argc > 1) {
        int parsed = atoi(argv[1]);
        if (parsed > 0)
            local_work_size = (size_t)parsed;
        else{ 
            // printf("::WARNING Invalid local_work_size argument '%s', using default %zu\n", argv[1], local_work_size);
        }
    }
    // printf("::INFO Using local_work_size: %zu\n", local_work_size);

    // -----------------------------
    // Reading the BMP Image
    // -----------------------------
    int error_code;
    BMPImage image;
    error_code = Read_BMP_From_File(&image, "assets/Image.bmp");
    if(error_code != 0) {
        // printf("Error when reading the BitMap image, exiting program!\n");
        return -1;
    }

    uint32_t size = image.infoHeader.image_height * image.infoHeader.image_width;
    RGB* rgbData = malloc(size * sizeof(RGB));
    convert_to_RGB(rgbData, image.imageData, size);
    // Write_To_BMP_File(&image, "output/original.BMP");

    // -----------------------------
    // Pthread Sort Timing
    // -----------------------------
    struct timeval pthread_start, pthread_end;
    gettimeofday(&pthread_start, NULL);

    Argument_t arg[N_THREADS];
    pthread_t threads[N_THREADS];

    // Prepare arguments for vertical sort (as in your code)
    for(int i = 0; i < N_THREADS; i++) {
        arg[i].rgb_array = rgbData;
        arg[i].width = image.infoHeader.image_width;
        arg[i].height = image.infoHeader.image_height;
        arg[i].ascending = 0;
        arg[i].start = image.infoHeader.image_width / N_THREADS * i;
        arg[i].end = image.infoHeader.image_width / N_THREADS * (i+1);
        if(i == N_THREADS-1)
            arg[i].end = image.infoHeader.image_width;
    }

    // Launch pthreads for vertical sort
    for(int i = 0; i < N_THREADS; i++)
        pthread_create(&threads[i], NULL, call_parallel_sort_vertical, &arg[i]);
    for(int i = 0; i < N_THREADS; i++)
        pthread_join(threads[i], NULL);

    gettimeofday(&pthread_end, NULL);
    double pthread_time = elapsed_ms(&pthread_start, &pthread_end);

    // Save pthread result
    convert_to_binary(image.imageData, rgbData, size);
    //Write_To_BMP_File(&image, "output/step_1.BMP");

    // -----------------------------
    // OpenCL Section (after pthreads)
    // -----------------------------
    struct timeval opencl_total_start, opencl_total_end;
    gettimeofday(&opencl_total_start, NULL);

    cl_int err;
    cl_uint n_platforms;
    cl_platform_id platform_id;
    err = clGetPlatformIDs(1, &platform_id, &n_platforms);
    if (err != CL_SUCCESS) {
        // printf("[ERROR] Error calling clGetPlatformIDs. Error code: %d\n", err);
        return 0;
    }

    cl_device_id device_id;
    cl_uint n_devices;
    err = clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_GPU, 1, &device_id, &n_devices);
    if (err != CL_SUCCESS) {
        // printf("[ERROR] Error calling clGetDeviceIDs. Error code: %d\n", err);
        return 0;
    }

    cl_context context = clCreateContext(NULL, n_devices, &device_id, NULL, NULL, NULL);

    const char* kernel_code = load_kernel_source("kernels/image-sort.cl", &error_code);
    if (error_code != 0) {
        // printf("Source code loading error!\n");
        return 0;
    }
    cl_program program = clCreateProgramWithSource(context, 1, &kernel_code, NULL, NULL);

    const char options[] = "-I src/BMPFILE.c";
    // printf("::INFO Building program\n");
    err = clBuildProgram(program, 1, &device_id, options, NULL, NULL);
    if (err != CL_SUCCESS) {
        // printf("Build error! Code: %d\n", err);
        size_t real_size;
        clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, 0, NULL, &real_size);
        char* build_log = (char*)malloc(sizeof(char) * (real_size + 1));
        clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, real_size + 1, build_log, &real_size);
        // printf("Real size : %zu\n", real_size);
        // printf("Build log : %s\n", build_log);
        free(build_log);
        return 0;
    }

    // printf("::INFO Creating kernel\n");
    cl_kernel kernel = clCreateKernel(program, "vertical_sort", NULL);

    RGB* host_buffer = (RGB*)malloc(size * sizeof(RGB));
    memcpy(host_buffer, rgbData, size * sizeof(RGB));

    // printf("::INFO Creating device buffer\n");
    cl_mem device_buffer = clCreateBuffer(
        context,
        CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR,
        size * sizeof(RGB),
        host_buffer,
        &err
    );
    if (err != CL_SUCCESS) {
        // printf("Unable to create buffer! Code: %d\n", err);
        return 0;
    }

    int ascending = 0;
    // printf("::INFO Setting kernel arguments\n");
    clSetKernelArg(kernel, 0, sizeof(cl_mem), (void*)&device_buffer);
    clSetKernelArg(kernel, 1, sizeof(int), (void*)&image.infoHeader.image_width);
    clSetKernelArg(kernel, 2, sizeof(int), (void*)&image.infoHeader.image_height);
    clSetKernelArg(kernel, 3, sizeof(int), (void*)&ascending);

    // printf("::INFO Creating command queue\n");
    cl_command_queue command_queue = clCreateCommandQueue(context, device_id, CL_QUEUE_PROFILING_ENABLE, NULL);

    // printf("::INFO Enqueuing write buffer\n");
    clEnqueueWriteBuffer(
        command_queue,
        device_buffer,
        CL_FALSE,
        0,
        size * sizeof(RGB),
        host_buffer,
        0,
        NULL,
        NULL
    );

    // Size specification
    size_t n_work_groups = image.infoHeader.image_width / local_work_size;
    if (n_work_groups == 0) n_work_groups = 1;
    size_t global_work_size = n_work_groups * local_work_size;

    // printf("::INFO Enqueuing kernel\n");
    cl_event kernel_event;
    clEnqueueNDRangeKernel(
        command_queue,
        kernel,
        1,
        NULL,
        &global_work_size,
        &local_work_size,
        0,
        NULL,
        &kernel_event
    );
    clWaitForEvents(1, &kernel_event);

    cl_ulong kernel_start, kernel_end;
    clGetEventProfilingInfo(kernel_event, CL_PROFILING_COMMAND_START, sizeof(kernel_start), &kernel_start, NULL);
    clGetEventProfilingInfo(kernel_event, CL_PROFILING_COMMAND_END, sizeof(kernel_end), &kernel_end, NULL);
    double kernel_time_ms = (kernel_end - kernel_start) * 1e-6;

    // printf("::INFO Reading from the device buffer\n");
    cl_event read_event;
    clEnqueueReadBuffer(
        command_queue,
        device_buffer,
        CL_TRUE,
        0,
        size * sizeof(RGB),
        host_buffer,
        0,
        NULL,
        &read_event
    );
    clWaitForEvents(1, &read_event);

    cl_ulong read_start, read_end;
    clGetEventProfilingInfo(read_event, CL_PROFILING_COMMAND_START, sizeof(read_start), &read_start, NULL);
    clGetEventProfilingInfo(read_event, CL_PROFILING_COMMAND_END, sizeof(read_end), &read_end, NULL);
    double read_time_ms = (read_end - read_start) * 1e-6;

    gettimeofday(&opencl_total_end, NULL);
    double opencl_total_time = elapsed_ms(&opencl_total_start, &opencl_total_end);

    // Save OpenCL result
    memcpy(rgbData, host_buffer, size * sizeof(RGB));
    convert_to_binary(image.imageData, rgbData, size);
    // Write_To_BMP_File(&image, "output/step_2.BMP");

    // -----------------------------
    // Print Timing Results
    // -----------------------------
    /*
    printf("\n==== Timing Results ====\n");
    printf("Pthread sort time:        %.3f ms\n", pthread_time);
    printf("OpenCL total time:        %.3f ms\n", opencl_total_time);
    printf("OpenCL kernel time:       %.3f ms\n", kernel_time_ms);
    printf("OpenCL read buffer time:  %.3f ms\n", read_time_ms);
    */
    printf("%d;", local_work_size);
    printf("%.3f;", pthread_time);
    printf("%.3f;", opencl_total_time);
    printf("%.3f;", kernel_time_ms);
    printf("%.3f", read_time_ms);
    sleep(1);
    // -----------------------------
    // Cleanup
    // -----------------------------
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseContext(context);
    clReleaseMemObject(device_buffer);
    clReleaseCommandQueue(command_queue);
    clReleaseDevice(device_id);

    free(image.imageData);
    free(rgbData);
    free(host_buffer);

    return 0;
}

// ---------------------------------
// Sorting Functions
// ---------------------------------
void sort_horizontal(RGB* rgb_array, uint32_t width, uint32_t height, uint8_t ascending, uint32_t start_y, uint32_t end_y)
{
    for(int y = start_y; y < end_y; y++)
    {
        int swapped = 0;
        for(int x = 1; x < width; x++)
        {
            RGB* previous = &(rgb_array[x + y * width - 1]);
            RGB* value = &(rgb_array[x + y * width]);
            uint32_t prev_sum = previous->Red + previous->Blue + previous->Green;
            uint32_t sum = value->Red + value->Blue + value->Green;
            if(ascending)
            {
                if(prev_sum > sum)
                {
                    swap_rgb(value, previous);
                    swapped = 1;
                }
            }
            else
            {
                if(prev_sum < sum)
                {
                    swap_rgb(value, previous);
                    swapped = 1;
                }
            }
        }
        if(swapped == 1)
        {
            y--;
        }
    }
}

void sort_vertical(RGB* rgb_array, uint32_t width, uint32_t height, uint8_t ascending, uint32_t start_x, uint32_t end_x)
{
    for(int x = start_x; x < end_x; x++)
    {
        uint8_t swapped = 0;
        for(int y = 1; y < height; y++)
        {
            RGB* previous = &(rgb_array[x + (y-1) * width]);
            RGB* value = &(rgb_array[x + y * width]);
            uint32_t prev_sum = previous->Red + previous->Blue + previous->Green;
            uint32_t sum = value->Red + value->Blue + value->Green;
            if(ascending)
            {
                if(prev_sum > sum)
                {
                    swap_rgb(value, previous);
                    swapped = 1;
                }
            }
            else
            {
                if(prev_sum < sum)
                {
                    swap_rgb(value, previous);
                    swapped = 1;
                }
            }
        }
        if(swapped == 1)
        {
            x--;
        }
    }
}

void swap_rgb(RGB* A, RGB* B)
{
    RGB tmp = *A;
    *A = *B;
    *B = tmp;
}

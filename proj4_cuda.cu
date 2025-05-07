#include <stdio.h>
#include <stdlib.h>
#include <cuda_runtime.h>
#include "device_launch_parameters.h"
#include "proj4.h"

__global__ void computeDiagonalsSumsKernel(unsigned char* input, unsigned char* output, int n, unsigned long target_sum) {
    int row = blockIdx.y * blockDim.y + threadIdx.y;
    int col = blockIdx.x * blockDim.x + threadIdx.x;

    if (row >= n || col >= n) return;

    // Checking Top-left to Bottom-right diagonal 
    unsigned long sum = 0;
    for (int offset = 0; row + offset < n && col + offset < n; offset++) {
        sum += input[(row + offset) * n + (col + offset)];
        if (sum == target_sum) {
            for (int i = 0; i <= offset; i++) {
                output[(row + i) * n + (col + i)] = input[(row + i) * n + (col + i)];
            }
        }
        if (sum > target_sum) {
            break;
        }
    }

    sum = 0;
    for (int offset = 0; row + offset < n && col - offset >= 0; offset++) {
        sum += input[(row + offset) * n + (col - offset)];
        if (sum == target_sum) {
            for (int i = 0; i <= offset; i++) {
                output[(row + i) * n + (col - i)] = input[(row + i) * n + (col - i)];
            }
        }
        if (sum > target_sum) {
            break;
        }
    }
}

extern "C" float diagonalSumsCUDA(grid* input, unsigned long target_sum, grid* output) {
    cudaEvent_t start, stop;  
    cudaEventCreate(&start);
    cudaEventCreate(&stop);
    cudaEventRecord(start); 
    output->n = input->n;
    output->p = (unsigned char**)malloc(output->n * sizeof(unsigned char*));
    if (!output->p) {
        printf("Memory allocation failed for output grid\n");
        return 0;
    }
    for (unsigned int i = 0; i < output->n; i++) {
        output->p[i] = (unsigned char*)calloc(output->n, sizeof(unsigned char));
        if (!output->p[i]) {
            printf("Memory allocation failed for output grid row %d\n", i);
            return 0;
        }
    }

    unsigned char* flat_input = (unsigned char*)malloc(input->n * input->n * sizeof(unsigned char));
    unsigned char* flat_output = (unsigned char*)malloc(input->n * input->n * sizeof(unsigned char));

    if (!flat_input || !flat_output) {
        printf("Memory allocation failed for input or output arrays\n");
        return 0;
    }

    for (unsigned int i = 0; i < input->n; i++) {
        for (unsigned int j = 0; j < input->n; j++) {
            flat_input[i * input->n + j] = input->p[i][j];
        }
    }

    unsigned char* d_input, * d_output;
    cudaMalloc(&d_input, input->n * input->n * sizeof(unsigned char));
    cudaMalloc(&d_output, input->n * input->n * sizeof(unsigned char));

    cudaMemcpy(d_input, flat_input, input->n * input->n * sizeof(unsigned char), cudaMemcpyHostToDevice);
    cudaMemset(d_output, 0, input->n * input->n * sizeof(unsigned char));

    dim3 blockSize(16, 16);
    dim3 gridSize((input->n + blockSize.x - 1) / blockSize.x,
                  (input->n + blockSize.y - 1) / blockSize.y);            
    computeDiagonalsSumsKernel<<<gridSize, blockSize>>>(d_input, d_output, input->n, target_sum);
    cudaError_t err = cudaGetLastError();
    if (err != cudaSuccess) {
        printf("CUDA error: %s\n", cudaGetErrorString(err));
        return 0;
    }

    cudaDeviceSynchronize();

    cudaMemcpy(flat_output, d_output, input->n * input->n * sizeof(unsigned char), cudaMemcpyDeviceToHost);

    for (unsigned int i = 0; i < output->n; i++) {
        for (unsigned int j = 0; j < output->n; j++) {
            output->p[i][j] = flat_output[i * output->n + j];  // Fix indexing error here
        }
    }

    free(flat_input);
    free(flat_output);
    cudaFree(d_input);
    cudaFree(d_output);
    cudaEventRecord(stop);
    cudaEventSynchronize(stop);
    float milliseconds = 0;
    cudaEventElapsedTime(&milliseconds, start, stop);
    return milliseconds;
}
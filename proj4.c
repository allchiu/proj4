#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include "proj4.h"

/**
 * This struct contains data members when working with multithreading.
 * The struct has two pointers: a pointer to the input grid (the original) and a pointer to the output grid
 * (the one after the diagonal sums have been calculated). The struct has a target_sum, the target sum the
 * diagonals need to add up to. The struct has a two integers: thread_id and total_threads, thread_id
 * stores the thread ID and the total_threads store the total threads being used. The struct has two unsigned integers: 
 * start_row and end_row. These two data members are used to track the beginning and ending rows for a specific thread
 * since this program is using multithreaded data parallelism.
 *
 */
typedef struct {
    grid *input;
    grid *output;
    unsigned long target_sum;
    int thread_id;
    int total_threads;
    unsigned int start_row;
    unsigned int end_row;
} thread_data;

void initializeGrid(grid *g, char *fileName) {
    FILE *file = fopen(fileName, "r");
    if (file == NULL) {
        fprintf(stderr, "Error opening file %s\n", fileName);
        exit(1);
    } // if

    // getting the number for numbers in one row of the grid
    unsigned int grid_size = 0;
    while (fgetc(file) != '\n') {
        grid_size++;
    } // while

    rewind(file);

    // assign member variables of the grid struct: pointer to the grid, allocating memory for row pointers
    g->n = grid_size;
    g->p = (unsigned char **)malloc(grid_size * sizeof(unsigned char *));

    // allocating memory for the columns in the grid and scans the value in via fscanf
    for (unsigned int row = 0; row < grid_size; row++) {
        g->p[row] = (unsigned char *)malloc(grid_size * sizeof(unsigned char));
        for (unsigned int col = 0; col < grid_size; col++) {
            fscanf_s(file, "%1hhu", &(g->p[row][col]));
        } // for
    } // for

    fclose(file);
} // initializeGrid

// Updated thread function using Windows API
DWORD WINAPI computeDiagonalSums(LPVOID arg) {
    thread_data *data = (thread_data *)arg;
    grid *input = data->input;
    grid *output = data->output;
    unsigned long target_sum = data->target_sum;

    for (unsigned int start_row = data->start_row; start_row < data->end_row; start_row++) {
        for (unsigned int start_col = 0; start_col < input->n; start_col++) {
            // Check diagonal from top-left to bottom-right
            unsigned long current_sum = 0;
            unsigned int diagonal_length = 0;
            while (start_row + diagonal_length < input->n && start_col + diagonal_length < input->n) {
                current_sum += input->p[start_row + diagonal_length][start_col + diagonal_length];
                if (current_sum == target_sum) {
                    for (unsigned int offset = 0; offset <= diagonal_length; offset++) {
                        output->p[start_row + offset][start_col + offset] =
                            input->p[start_row + offset][start_col + offset];
                    } // for
                } // if
                if (current_sum > target_sum) {
                    diagonal_length = input->n;
                } else {
                    diagonal_length++;
                } // if-else
            } // while

            // Check diagonal from top-right to bottom-left
            current_sum = 0;
            diagonal_length = 0;
            while (start_row + diagonal_length < input->n && start_col >= diagonal_length) {
                current_sum += input->p[start_row + diagonal_length][start_col - diagonal_length];
                if (current_sum == target_sum) {
                    for (unsigned int offset = 0; offset <= diagonal_length; offset++) {
                        output->p[start_row + offset][start_col - offset] =
                            input->p[start_row + offset][start_col - offset];
                    } // for
                } // if
                if (current_sum > target_sum) {
                    diagonal_length = input->n;
                } else {
                    diagonal_length++;
                } // if-else
            } // while
        } // for
    } // for

    return 0; // Windows thread return value (instead of NULL in pthreads)
} // computeDiagonalSums

// Updated diagonalSums function for Windows threading
void diagonalSums(grid *input, unsigned long target_sum, grid *output, int num_threads) {
    output->n = input->n;
    output->p = (unsigned char **)malloc(output->n * sizeof(unsigned char *));
    for (unsigned int row = 0; row < output->n; row++) {
        output->p[row] = (unsigned char *)calloc(output->n, sizeof(unsigned char));
    } // for

    if (num_threads == 1) {
        // single-threaded
        thread_data data;
        data.input = input;
        data.output = output;
        data.target_sum = target_sum;
        data.start_row = 0;
        data.end_row = input->n;
        computeDiagonalSums(&data);
    } else {
        // multithreaded processing using data parallelism
        HANDLE *threads = (HANDLE *)malloc(num_threads * sizeof(HANDLE));
        thread_data *thread_args = (thread_data *)malloc(num_threads * sizeof(thread_data));

        unsigned int rows_per_thread = input->n / num_threads;
        for (int thread_num = 0; thread_num < num_threads; thread_num++) {
            thread_args[thread_num].input = input;
            thread_args[thread_num].output = output;
            thread_args[thread_num].target_sum = target_sum;
            thread_args[thread_num].start_row = thread_num * rows_per_thread;
            thread_args[thread_num].end_row = (thread_num == num_threads - 1) ? 
                input->n : (thread_num + 1) * rows_per_thread;

            // Create the thread using CreateThread
            threads[thread_num] = CreateThread(
                NULL, 0, computeDiagonalSums, &thread_args[thread_num], 0, NULL);
        } // for

        // Wait for all threads to finish using WaitForSingleObject
        for (int thread_num = 0; thread_num < num_threads; thread_num++) {
            WaitForSingleObject(threads[thread_num], INFINITE);
            CloseHandle(threads[thread_num]); // Clean up thread handle
        } // for

        // Free allocated memory for threads and thread_args
        free(threads);
        free(thread_args);
    } // if-else
} // diagonalSums

void writeGrid(grid *g, char *fileName) {
    FILE *file = fopen(fileName, "w");
    if (!file) {
        fprintf(stderr, "Error opening file %s\n", fileName);
        exit(1);
    } // if

    for (unsigned int row = 0; row < g->n; row++) {
        for (unsigned int col = 0; col < g->n; col++) {
            fprintf(file, "%hhu", g->p[row][col]);
        } // for
        fprintf(file, "\n");
    } // fpr

    fclose(file);
} // writeGrid

void freeGrid(grid *g) {
    for (unsigned int row = 0; row < g->n; row++) {
        free(g->p[row]);
    } // for
    free(g->p);
} // freeGrid

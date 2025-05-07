#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <windows.h>
#include "proj4.h"

/*
 * Do not modify anything in this file.
 */

/*
 * Helper function for main.
 * Check for some errors and print error messages
 * for command line arguments passed to main.
 * If an error is found, the program terminates.
 */
void errorCheck(int argc, char **argv) {
    bool errorFound = false;

    if (argc != 6) {
        printf("Usage error: proj4.exe inputFile outputFile gpuOutputFile s t\n");
        errorFound = true;
    } else {
        FILE *f = fopen(argv[1], "r");
        if (!f) {
            printf("Error accessing %s in the present working directory\n", argv[1]);
            errorFound = true;
        } else {
            fclose(f);
        }

        int t = atoi(argv[5]);
        if (t < 1 || t > 3) {
            printf("Error: t must be between 1 and 3 (inclusive)\n");
            errorFound = true;
        }
    }

    if (errorFound) {
        exit(0);
    }
}

double get_time_in_seconds() {
  static LARGE_INTEGER frequency;
  static BOOL initialized = FALSE;
  if (!initialized) {
      QueryPerformanceFrequency(&frequency);
      initialized = TRUE;
  }

  LARGE_INTEGER counter;
  QueryPerformanceCounter(&counter);
  return (double)counter.QuadPart / frequency.QuadPart;
}


/*
 * This program should be compiled to ./proj4.out using the provided
 * Makefile, and it will process five command line arguments.
 *   ./proj.out input.txt output.txt s t
 *      input.txt is the name of a file in the present working directory that
 *        contains a n-by-n grid of digits (1 through 9),
 *        where n >= 1.
 *      output.txt is the name of a file in the present working directory to save
 *        the output of all of the diagonal sums. If the file does not exist,
 *        then this file will be created in the present working directory.
 *      s is the sum for the diagonal sums.
 *      t is the number of threads (1 <= t <= 3) to use
 *        to compute the diagonal sums. If t is 1, then only the
 *        main thread will be used. If 2 <= t <= 3, then the main
 *        thread and (t - 1) POSIX thread(s) will be used to compute
 *        the diagonal sums.
 * This program will only time the call to diagonalSums.
 *
 */
int main(int argc, char ** argv){
  errorCheck(argc, argv);
  char * inputFile = argv[1];
  char * outputFile = argv[2];
  char * gpuOutputFile = argv[3];
  unsigned long sum = (unsigned long) atol(argv[4]);
  int t = atoi(argv[5]);
  grid g, diagonalSumsOutput;
  initializeGrid(&g, inputFile);

  printf("Computing the diagonal sums equal to %ld in a %d-by-%d grid using %d thread(s).\n",
         sum, g.n, g.n, t);
  
  double start = get_time_in_seconds();
  diagonalSums(&g, sum, &diagonalSumsOutput, t);
  double end = get_time_in_seconds();
  double elapsed = end - start;  
  printf("Elapsed time for computing the diagonal sums using %d CPU thread(s): %lf seconds.\n", t, elapsed);


  grid gpuOutput;
  printf("\nRunning CUDA version...\n");
  double startGPU = get_time_in_seconds();
  float timingUsingCUDAEvent = diagonalSumsCUDA(&g, sum, &gpuOutput);
  double endGPU = get_time_in_seconds();
  double elapsedGPU = endGPU - startGPU;
  printf("Elapsed time for computing the diagonal sums using %d GPU thread(s): %lf seconds.\n", t, elapsedGPU);
  printf("Elapsed time using CUDA Event timer for computing the diagonal sums using %d GPU thread(s): %f seconds.\n", t, timingUsingCUDAEvent / 1000.0);
  printf("CPU: Writing the diagonal sums equal to %ld to the file %s.\n", sum, outputFile);
  printf("GPU: Writing the diagonal sums equal to %ld to the file %s.\n", sum, gpuOutputFile);
  writeGrid(&gpuOutput, gpuOutputFile);
  writeGrid(&diagonalSumsOutput, outputFile);
  freeGrid(&g);
  freeGrid(&diagonalSumsOutput);
  freeGrid(&gpuOutput);
  printf("Program is complete. Goodbye!\n");
  return 0;
}

/*! @example 
 @brief A program that implements data parallel variant\n
 we run the example as ./dataparallel <len> <tasks> \n
 where
 \param len  := length of vector\n
 \param tasks := number of workders TAOs\n
*/
#include "xitao.h"
#include <iostream>
#include <chrono>
#include <cstring>
#include <omp.h>
using namespace xitao;
int main(int argc, char *argv[]) {
  // init XiTAO runtime
  int i = 0;
  int end = 10;
  int N       = (argc > 1) ? atoi(argv[1]) : 1000;
  int workers = (argc > 2) ? atoi(argv[2]) : 4;
  int **A, **B, **C;
  A = new int*[N];
  B = new int*[N];
  C = new int*[N];
  for(int r = 0 ; r < N; ++r) {
    A[r] = new int[N];
    std::memset(A[r], 0, sizeof(int) * N);
    B[r] = new int[N];
    std::memset(B[r], 0, sizeof(int) * N);
    C[r] = new int[N];
    std::memset(C[r], 0, sizeof(int) * N);
  }
  gotao_init();
  gotao_start();
  std::chrono::time_point<std::chrono::system_clock> start_time, end_time;
  start_time = std::chrono::system_clock::now();
  __xitao_vec_region(workers, i, N, 
    for (int j = 0; j < N; j++) 
     { 
       C[i][i] = 0; 
       for (int k = 0; k < N; k++) 
         C[i][j] += A[i][k] *  
                    B[k][j]; 
     }
  );
  gotao_fini();
  end_time = std::chrono::system_clock::now();
  std::chrono::duration<double> elapsed_seconds = end_time - start_time;
  std::cout << "Total time XiTAO: " << elapsed_seconds.count()  << std::endl;
  std::cout << "Total successful steals: " << tao_total_steals << std::endl;
  start_time = std::chrono::system_clock::now();
#pragma omp parallel for num_threads(workers) 
  for(int i = 0; i < N; ++i)
    for (int j = 0; j < N; ++j) 
     { 
       C[i][i] = 0; 
       for (int k = 0; k < N; ++k) 
         C[i][j] += A[i][k] *  
                    B[k][j]; 
     }
  
  end_time = std::chrono::system_clock::now();
  elapsed_seconds = end_time - start_time;
  std::cout << "Total time OpenMP: " << elapsed_seconds.count()  << std::endl;
}

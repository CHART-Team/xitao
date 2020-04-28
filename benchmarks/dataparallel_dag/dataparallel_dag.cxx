/*! @example 
 @brief A program that implements data parallel variant\n
 we run the example as ./dataparallel <veclength> <TAOwidth> <0:static, 1:dynamic> \n
 where
 \param veclength  := length of vector\n
 \param TAOwidth := number of workders\n
 \param internal scheduling := 0:static or 1:dynamic\n
 \param TAO block length := the block length pertaining to the internal TAO \n
 \param PTT training attempts:= the number of iterations for multiparallel region\n
*/
#include "xitao.h"
#include <iostream>
#include <chrono>
#include <cstring>
#include <omp.h>
using namespace xitao;

// The Matrix Reset TAO (contains work and data)
class ResetMatTAO : public AssemblyTask {
  // the data
  int **A;
  int **B;
  int **C;
  int N;
public:
  ResetMatTAO(int **_A, int **_B, int **_C, int _N) :
   AssemblyTask(1), A(_A), B(_B), C(_C), N(_N) { }
  void execute(int nthread) {
    int tid = nthread - leader; 
    if(tid == 0){
      for(int r = 0 ; r < N; ++r) {
        A[r] = new int[N];
        std::memset(A[r], 0, sizeof(int) * N);
        B[r] = new int[N];
        std::memset(B[r], 0, sizeof(int) * N);
        C[r] = new int[N];
        std::memset(C[r], 0, sizeof(int) * N);
      }
    }

  }
  void cleanup () { }
};


int main(int argc, char *argv[]) {  
  if(argc < 7) {
    std::cout << "./dataparallel <veclength> <workers> <0:static, 1:dynamic> <TAO block length> <TAO initial width> <PTT training attempts>" << std::endl; 
    return 0;
  }  
  int i = 0;                                        
  int end = 10;
  // problem size default is 1000
  int N       = (argc > 1) ? atoi(argv[1]) : 1000; 
  // parallelism default is 4
  int workers = (argc > 2) ? atoi(argv[2]) : 4;    
  // static or dynamic scheduling default is static
  int sched   = (argc > 3) ? atoi(argv[3]) : 0;      
  // the fine grain TAO block size (in case of multiparallel SPMD region)
  int block_length   = (argc > 4) ? atoi(argv[4]) : 0;      
  // the fine grain TAO block size (in case of multiparallel SPMD region)
  int tao_width   = (argc > 5) ? atoi(argv[5]) : 0;        
  // the number of iterations for multiparallel region
  int iter_count     = (argc > 6) ? atoi(argv[6]) : 1;      
  std::cout << "N: " << N << std::endl;           
  std::cout << "P: " << workers << std::endl;
  std::cout << "Block: " << block_length << std::endl;
  int **A, **B, **C;
  A = new int*[N];
  B = new int*[N];
  C = new int*[N];
  gotao_init_hw(workers, -1 , -1);
  ResetMatTAO* resetTAO = new ResetMatTAO(A, B, C, N);
  ResetMatTAO* headTAO = resetTAO;
  for (int iter = 0; iter < iter_count; ++iter) {
    auto vec = __xitao_vec_non_immediate_multiparallel_region(tao_width, i, N, sched, block_length, 
      for (int j = 0; j < N; j++) 
       { 
         C[i][i] = 0; 
         for (int k = 0; k < N; k++) 
           C[i][j] += A[i][k] *  
                      B[k][j]; 
       }
    );
    for(auto dataparallelNode : vec) {
      resetTAO->make_edge(dataparallelNode);
    }

    resetTAO = new ResetMatTAO(A, B, C, N);
    for(auto dataparallelNode : vec) {
      dataparallelNode->make_edge(resetTAO);
    }
  }
  gotao_push(headTAO);
  gotao_start();
  gotao_fini();
  std::cout << "Total successful steals: " << tao_total_steals << std::endl;      
}

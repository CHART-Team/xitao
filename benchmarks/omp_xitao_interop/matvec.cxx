#include "matvec_tao.h"
#include "xitao.h"
#include <omp.h>
using namespace xitao;
int
main(int argc, char *argv[])
{
  double *A, *X, *B; 
  if(argc != 2) {
    std::cout << "./a.out <veclength>" << std::endl; 
    return 0;
  }

  // the matrix dim size
  size_t N = atoll(argv[1]);
  
  //allocate odd numbered places to OpenMP 
  int err = setenv("OMP_PLACES", "{1},{3},{5},{7}", 1);
  if(err) { 
    std::cout << "Failed to set OMP_PLACES with error: " << err << std::endl;
    exit(0);
  }
  
  // assign even numbered places to XiTAO
  cpu_set_t cpu_;
  CPU_ZERO(&cpu_);
  for(int i = 0; i < 8; i+=2) {
    CPU_SET(i, &cpu_);
  } 
  set_xitao_mask(cpu_);
  
  // assign the full set of CPUs as resource hint
  int width = CPU_COUNT(&cpu_);

  // no topologies in this version
  A = new double[N * N];
  X = new double[N];
  B = new double[N];

  // initialize the vectors with some numbers
  for(size_t i = 0; i < N * N; i++){
    A[i] = (double) (i+1);
    if(i < N) X[i] = (double) (i+1);
  }
  size_t xitao_work_size = N / 2;
  
  size_t openmp_work_size = N / 2 + N % 2;
  
  std::cout << "XiTAO work size: " << xitao_work_size << std::endl;
  std::cout << "OpenMP work size: " << openmp_work_size << std::endl;
  // assign the work pointers 
  double* A_xitao = A;
  double* A_omp   = A + (xitao_work_size * N);
  double* B_xitao = B;
  double* B_omp   = B + xitao_work_size;

  MatVecTAO* vm = new MatVecTAO(A_xitao, X, B_xitao, xitao_work_size, N, width);
#pragma omp parallel 
{
#pragma omp single nowait 
{
#pragma omp taskloop shared(A_omp, X, B_omp, openmp_work_size, N)
  for(size_t i = 0; i < openmp_work_size; ++i) { 
    for(size_t j = 0; j < N; ++j) {  
      B_omp[i] = A_omp[i * N + j] * X[j];
    }
  }

}
#pragma omp single nowait
{
  // init XiTAO runtime 
  gotao_init();
  // push the TAO
  gotao_push(vm);
  //Start the TAODAG exeuction
  gotao_start();
  //Finalize and claim resources back
  gotao_fini();
}
}

  std::cout << "Completed ... " << std::endl;
}

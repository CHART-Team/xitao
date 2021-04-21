/* BSD 3-Clause License

* Copyright (c) 2019-2021, contributors
* All rights reserved.

* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:

* 1. Redistributions of source code must retain the above copyright notice, this
*    list of conditions and the following disclaimer.

* 2. Redistributions in binary form must reproduce the above copyright notice,
*    this list of conditions and the following disclaimer in the documentation
*    and/or other materials provided with the distribution.

* 3. Neither the name of the copyright holder nor the names of its
*    contributors may be used to endorse or promote products derived from
*    this software without specific prior written permission.

* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

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
  xitao_init();
  // push the TAO
  xitao_push(vm);
  //Start the TAODAG exeuction
  xitao_start();
  //Finalize and claim resources back
  xitao_fini();
}
}

  std::cout << "Completed ... " << std::endl;
}

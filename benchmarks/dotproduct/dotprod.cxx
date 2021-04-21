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

/*! @example 
 @brief A program that calculates dotproduct of random two vectors in parallel\n
 we run the example as ./dotprod.out <len> <width> <block> \n
 where
 \param len  := length of vector\n
 \param width := width of TAOs\n
 \param block := how many elements to process per TAO
*/
#include "taos_dotproduct.h"
#include "xitao.h"
using namespace xitao;
int
main(int argc, char *argv[])
{
  double *A, *B, *C, D; 
  if(argc < 4) {
    std::cout << "./a.out <veclength> <TAOwidth> <blocklength>" << std::endl; 
    return 0;
  }

  int len = atoi(argv[1]);
  int width = atoi(argv[2]);
  int block = atoi(argv[3]);

  // For simplicity, only support only perfect partitions
  if(len % block){  
    std::cout << "len is not a multiple of block!" << std::endl;
    return 0;
  }
  
  //cpu_set_t cpu_;
  //CPU_ZERO(&cpu_);
  //for(int i = 0; i < 8; i+=1) {
  //  CPU_SET(i, &cpu_);
  //} 
  //set_xitao_mask(cpu_);
  
  // no topologies in this version
  A = new double[len];
  B = new double[len];
  C = new double[len];

  // initialize the vectors with some numbers
  for(int i = 0; i < len; i++){
    A[i] = (double) (i+1);
    B[i] = (double) (i+1);
  }

  // init XiTAO runtime 
  xitao_init(argc, argv);
  
  // create numvm TAOs 
  int numvm = len / block;

  // static or dynamic internal TAO scheduler
#ifdef STATIC
  VecMulSta *vm[numvm];  
#else
  VecMulDyn *vm[numvm];
#endif  
  VecAdd *va = new VecAdd(C, &D, len, width);
  
  // Create the TAODAG
  for(int j = 0; j < numvm; j++){
#ifdef STATIC
    vm[j] = new VecMulSta(A+j*block, B+j*block, C+j*block, block, width);
#else
    vm[j] = new VecMulDyn(A+j*block, B+j*block, C+j*block, block, width);
#endif
    //Create an edge
    vm[j]->make_edge(va);
    //Push current root to assigned queue
    xitao_push(vm[j], j % xitao_nthreads);
  } 
  //Start the TAODAG exeuction
  xitao_start();
  //Finalize and claim resources back
  xitao_fini();
  std::cout << "Result is " << D << std::endl;
  std::cout << "Done!\n";
  std::cout << "Total successful steals: " << tao_total_steals << std::endl;
}

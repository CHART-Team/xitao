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

#include "fibonacci.h"
/*! @example 
 @brief A a program that calculates the Nth Fibonacci term in parallel using XiTAO\n
 we run the example as ./fibonacci <fibonacci_term_number> [grain_size] \n
 where
 \param fibonacci_term_number  := the Fib term to be found\n
 \param grain_size := when to stop creating TAOs (the higher the coarser creation)\n
*/
int main(int argc, char** argv) {
  // accept the term number as argument
  if(argc < 2) {
    // prompt for input
    std::cerr << "Usage: ./fibonacci <fibonacci_term_number> [grain_size]" << std::endl;
    // return error
    return -1;
  }
  // fetch the number
  uint32_t num = atoi(argv[1]);
  // you can optionally pass coarsening degree
  grain_size = (argc > 2)? atoi(argv[2]) : num - 2;
  // extra guard against error value decided by program
  if(grain_size <= 2) grain_size = 2;
  // error check
  if(num > MAX_FIB or num < 0) {
    // print error
    std::cerr << "Bounds error! acceptable range for term: 0 <= term <= " << MAX_FIB << std::endl;
    // return error
    return -1;  
  }  
  // error check
  if(grain_size >= num or grain_size < 2) {
    // print error
    std::cerr << "Bounds error! acceptable range for grain size: 2 <= grain <= " << num << std::endl;
    // return error
    return -1;  
  }  
  xitao_set_num_threads(4);
  // initialize the XiTAO runtime system 
  xitao_init();           
  // recursively build the DAG                            
  auto parent = buildDAG(num);  
  // start the timer
  xitao::start("Time in XiTAO");
  // fire the XiTAO runtime system                    
  xitao_start();          
  // end the XiTAO runtime system                       
  xitao_fini();
  // stop the timer
  xitao::stop("Time in XiTAO");
  // start the timer
  xitao::start("Time in OpenMP");
  // call the serial code  
  size_t val;
#pragma omp parallel 
{
#pragma omp single
{ 
  val = fib_omp(num);
}
}
  // stop the timer
  xitao::stop("Time in OpenMP");
  // check if serial and parallel values agree
  if(val != parent->val) {
    // print out the error
    std::cout << "Value error! Serial= " << val <<". Prallel= " << parent->val << std::endl;
    // return failure 
    return -1;
  }
  // print out the result
  std::cout << "The " << num <<"th Fibonacci term = " << parent->val << std::endl;
  // return success
  return 0;
}

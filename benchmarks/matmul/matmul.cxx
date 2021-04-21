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

#include "matmul.h"
#include <vector>
#include <iostream>
#include <cmath>
using namespace std;
//#define VERIFY
int main(int argc, char** argv)
{
    int use_omp = 0;
    if(argc < 3) { 
      cout << "usage: ./matmul matrix_dim_size leaf_block_size [use_omp use_sta_in_xitao use_workload_hint]" <<endl;
      exit(0);
    }
    if(argc > 1) N = atoi(argv[1]);
    if(argc > 2) leaf = atoi(argv[2]);
    if(argc > 3) use_omp = atoi(argv[3]);
    if(argc > 4) use_sta = atoi(argv[4]);
    if(argc > 5) use_workload_hint = atoi(argv[5]);

    vector<real_t> a (N * N);
    vector<real_t> b (N * N);
    vector<real_t> c (N * N);

    xitao::config::formatted_print("N", N);
    xitao::config::formatted_print("Leaf Size", leaf);
    xitao::config::formatted_print("Using STA", use_sta);
    xitao::config::formatted_print("Using workload hint", use_workload_hint);
    xitao::config::formatted_print("Using OpenMP", use_omp);

    for(int i = 0; i < N * N ; ++i) {
      a[i] = (real_t)rand() / RAND_MAX;
      a[i] = -10.0 + a[i] * (10.0 + 10.0);

      b[i] = (real_t)rand() / RAND_MAX;
      b[i] = -10.0 + b[i] * (10.0 + 10.0);       
    
      c[i] = 0.0;
    }
    if(use_omp != 0) {
      xitao::start("Time in OpenMP");
      matmul_omp_parfor(&a[0], &b[0], &c[0], N, N);
      xitao::stop("Time in OpenMP");
    } else { 
        // initialize the XiTAO runtime system 
        xitao_init(argc, argv);
        std::cout << "Building the DAG ..." << std::endl;
        

        // recursively build the DAG
        divide(&a[0], &b[0], &c[0], N);
        std::cout << "DAG is ready, execute ..." << std::endl;
        
        // start the timer
        xitao::start("Time in XiTAO");
        // fire the XiTAO runtime system                    
        xitao_start();
        std::cout << "Started ... " << std::endl;    
        // end the XiTAO runtime system                       
        xitao_fini();
        std::cout << "Finished!" << std::endl;        
        // stop the timer
        xitao::stop("Time in XiTAO");
    }
#ifdef VERIFY
    vector<real_t> c_serial (N * N);
    matmul_serial(&a[0], &b[0], &c_serial[0], N, N);
    
    //! Verify result
    double pDif = 0, pNrm = 0;
    int count = 0;
    for (size_t b=0; b<c_serial.size(); b++) {
     // cout << c[b] << "," << c_serial[b] <<endl;    
      auto val = (c[b] - c_serial[b]) * (c[b] - c_serial[b]);
      pDif += val;
      pNrm += c_serial[b] * c_serial[b];                        
    }
    xitao::config::formatted_print("Rel. L2 Error (p)", sqrt(pDif/pNrm));
#endif
    // print out the result
    // if(is_sorted(A.begin(), A.end())) cout << "Success" << endl;
    // else cout << "Failure " << endl;
    // return success
    return 0;
}

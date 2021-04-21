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

#ifndef MATMUL_H
#define MATMUL_H
#include "xitao.h"


#ifdef SINGLE
typedef float real_t;
#else
typedef double real_t;
#endif

typedef uint32_t len_t;

len_t leaf = 32;
len_t N = 256; 
int use_omp = 0;
int use_sta = 0;
uint32_t use_workload_hint = 0;

template<class arr>
void print_matrix(arr mat, len_t n) {
  cout << "**********************";
  for(int i = 0; i < n; ++i) {
    for(int j = 0; j < n; ++j) {
      cout << mat[i * n + j] << ",";
    }
    cout << endl;
  }
  cout << "**********************";

}
void matmul_serial(real_t* a, real_t* b, real_t* c, len_t block, len_t dim) {
  for(len_t i = 0; i < block; ++i) {
    for(len_t k = 0; k < block; ++k) {
      for(len_t j = 0; j < block; ++j) {
        c[i * dim + j] += a[i * dim + k] * b[k * dim + j];
      }
    }
  }
}



void matmul_omp_parfor(real_t* a, real_t* b, real_t* c, len_t block, len_t dim) {
#pragma omp parallel for
  for(len_t i = 0; i < block; ++i) {
    for(len_t k = 0; k < block; ++k) {
      for(len_t j = 0; j < block; ++j) {
        c[i * dim + j] += a[i * dim + k] * b[k * dim + j];
      }
    }
  }
}

// the Merge MatMul TAO (Every TAO class must inherit from AssemblyTask)
class MatMulTAO : public AssemblyTask {
public:
    real_t* a;
    real_t* b;
    real_t* c;
    real_t** deps;
    len_t n;
    atomic<len_t> next;

    // the tao construction. resource hint 1
    MatMulTAO(real_t* a, real_t* b, real_t* c, real_t** deps, len_t n): 
                a(a), b(b), c(c), deps(deps), n(n), AssemblyTask(1) { 
      if(use_workload_hint != 0) workload_hint = n;
      for(int i = 0; i < n; ++i) c[i] = 0;
      next = 0;
    }    
    
    // the work function
    void execute(int nthread) {
      if(n <= leaf) {
        len_t i = next++;
        while(i < n) {
          for(len_t k = 0; k < n; ++k) {
            for(len_t j = 0; j < n; ++j) {
              c[i * n + j] += a[i * N + k] * b[k * N + j];
            }
          }
          i = next++;
        }
      } else {
        len_t block = n / 2;
        len_t i = next++;
        auto& c1 = deps[0];
        auto& c2 = deps[1];
        auto& c3 = deps[2];
        auto& c4 = deps[3];
        auto& c5 = deps[4];
        auto& c6 = deps[5];
        auto& c7 = deps[6];
        auto& c8 = deps[7];  
        while(i < block) { 
          for(int j = 0; j < block; ++j) {
            c[i * n + j] = c1[i * block + j] + c2[i * block + j];
          }
          for(int j = 0; j < block; ++j) {
            c[i * n + j + block] = c3[i * block + j] + c4[i * block + j];
          }
          for(int j = 0; j < block; ++j) {
            c[(i + block) * n + j]= c5[i * block + j] + c6[i * block + j];
          }
          for(int j = 0; j < block; ++j) {
            c[(i + block) * n + j + block] = c7[i * block + j] + c8[i * block + j];
          } 
          i = next++;
        }
      }
    }
    
    void cleanup() { 
      if(deps != NULL) {
        for(int i = 0; i < 8; ++i) delete[] deps[i];
        delete[] deps;
      }
    }
};

void divide(real_t* a, real_t* b, real_t* c, len_t n, len_t offset_x = 0, len_t offset_y = 0, MatMulTAO* parent = NULL) {
  MatMulTAO* matmul_tao;
  float sta = float(offset_x * N + offset_y) / (N * N);
  assert(sta < 1.0f);
  if(n <= leaf) {
    matmul_tao = new MatMulTAO(a, b, c, NULL, n);
    matmul_tao->set_sta(sta);
    xitao_push(matmul_tao);
  } else {
    real_t** sub_c = new real_t*[8];
    real_t* a11 = a;
    real_t* a12 = a + n / 2;
    real_t* a21 = a + N * n / 2;
    real_t* a22 = a + N * n / 2 + n / 2;

    real_t* b11 = b;
    real_t* b12 = b + n / 2;
    real_t* b21 = b + N * n / 2;
    real_t* b22 = b + N * n / 2 + n / 2;

    sub_c[0]  = new real_t[n / 2 * n / 2];
    sub_c[1]  = new real_t[n / 2 * n / 2];
    sub_c[2]  = new real_t[n / 2 * n / 2];
    sub_c[3]  = new real_t[n / 2 * n / 2];

    sub_c[4] = new real_t[n / 2 * n / 2];
    sub_c[5] = new real_t[n / 2 * n / 2];
    sub_c[6] = new real_t[n / 2 * n / 2];
    sub_c[7] = new real_t[n / 2 * n / 2];

    matmul_tao = new MatMulTAO(a, b, c, sub_c, n);
    matmul_tao->set_sta(sta);

    divide(a11, b11, sub_c[0], n / 2, offset_x, offset_y, matmul_tao);
    divide(a12, b21, sub_c[1], n / 2, offset_x, offset_y, matmul_tao);

    divide(a11, b12, sub_c[2], n / 2, offset_x, offset_y + n / 2, matmul_tao);
    divide(a12, b22, sub_c[3], n / 2, offset_x, offset_y + n / 2, matmul_tao);
    
    divide(a21, b11, sub_c[4], n / 2, offset_x + n / 2, offset_y, matmul_tao);
    divide(a22, b21, sub_c[5], n / 2, offset_x + n / 2, offset_y, matmul_tao);

    divide(a21, b12, sub_c[6], n / 2, offset_x + n / 2, offset_y + n / 2, matmul_tao);
    divide(a22, b22, sub_c[7], n / 2, offset_x + n / 2, offset_y + n / 2, matmul_tao);
  }
  if(parent) matmul_tao->make_edge(parent);
}




#endif

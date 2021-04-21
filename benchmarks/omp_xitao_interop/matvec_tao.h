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

/*! \file 
@brief Contains the TAOs needed for the dot product example
*/
#include "tao.h"
#include <chrono>
#include <iostream>
#include <atomic>

using namespace std;

/*! this TAO will take two vectors and multiply them. 
This TAO implements internal dynamic scheduling.*/
class MatVecTAO : public AssemblyTask 
{
public:   
  //! VecMulDyn TAO constructor. 
  /*!
    \param _A is the A vector
    \param _B is the B vector
    \param _C is the Result vector
    \param _nrows is the number of rows to work on
    \param _N is the matrix dimension size
    \param width is the number of resources used by this TAO
    The constructor computes the number of elements per thread and overdecomposes the domain using PSLACK parameter
    In this simple example, we do not instatiate a dynamic scheduler (yet)
  */  
  MatVecTAO(double *_A, double  *_X, double *_B, int _nrows, int _N, 
            int width) : AssemblyTask(width), A(_A), X(_X), B(_B), nrows(_nrows), N(_N) 
  {  
     i = 0; // initialize the atomic variable for dynamic scheduling withing the SPMD region 
  }

  //! Inherited pure virtual function that is called by the runtime to cleanup any resources (if any), held by a TAO. 
  void cleanup(){ 
    delete[] A;
    delete[] X;
  }

  //! Inherited pure virtual function that is called by the runtime upon executing the TAO. 
  /*!
    \param threadid logical thread id that executes the TAO
    This assembly can work totally asynchronously
  */
  void execute(int threadid)
  {
   //  int tid = threadid - leader;
    while(i++ < nrows){
      for (size_t j = 0; j < N; ++j) { 
        B[i] = A[i*N + j] * X[j]; 
      } 
    }
  }
  double *A;        /*!< TAO implementation specific double array that holds the A vector */
  double *X;        /*!< TAO implementation specific double array that holds the X vector */
  double *B;        /*!< TAO implementation specific double array that holds the result vector */
  size_t nrows;     /*!< TAO implementation specific integer that holds the vector length */
  size_t N;         /*!< TAO implementation specific integer that holds the vector length */
  atomic<size_t> i; /*!< TAO implementation specific atomic variable to provide thread safe tracker of the number of processed blocks */
};


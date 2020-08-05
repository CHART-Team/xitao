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


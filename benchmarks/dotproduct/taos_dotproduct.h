/*! \file 
@brief Contains the TAOs needed for the dot product example
*/
#include "tao.h"
#include <chrono>
#include <iostream>
#include <atomic>

extern "C" {

#include <stdio.h>
#include <stdlib.h> 
#include <unistd.h>

}

// parallel slackness        
#define PSLACK 8  
//#define CRIT_PERF_SCHED
using namespace std;

/*! this TAO will take two vectors and multiply them. 
This TAO implements internal static scheduling.*/
class VecMulSta : public AssemblyTask 
{
public:  
  //! VecMulSta TAO constructor. 
  /*!
    \param _A is the A vector
    \param _B is the B vector
    \param _C is the Result vector
    \param _len is the length of the vector 
    \param width is the number of resources used by this TAO
    The constructor computes the number of elements per thread.
    In this simple example, we do not instatiate a dynamic scheduler (yet)
  */  
  VecMulSta(double *_A, double  *_B, double *_C, int _len, 
      int width) : A(_A), B(_B), C(_C), len(_len), AssemblyTask(width) 
  {  
     if(len % width) std::cout <<  "Warning: blocklength is not a multiple of TAO width\n";
     block = len / width;
  }

  //! Inherited pure virtual function that is called by the runtime to cleanup any resources (if any), held by a TAO. 
  void cleanup() {  
    
  }
  //! Inherited pure virtual function that is called by the runtime upon executing the TAO
  /*!
    \param threadid logical thread id that executes the TAO
  */
  void execute(int threadid)
  {
    int tid = threadid - leader; 
    for(int i = tid*block; (i < len)  && (i < (tid+1)*block); i++)
          C[i] = A[i] * B[i];
  }
  
  int block; /*!< TAO implementation specific integer that holds the number of blocks per TAO */
  int len;   /*!< TAO implementation specific integer that holds the vector length */
  double *A; /*!< TAO implementation specific double array that holds the A vector */
  double *B; /*!< TAO implementation specific double array that holds the B vector */
  double *C; /*!< TAO implementation specific double array that holds the result vector */
};

#if defined(CRIT_PERF_SCHED)
float VecMulSta::time_table[XITAO_MAXTHREADS][XITAO_MAXTHREADS]; 
#endif

/*! this TAO will take two vectors and multiply them. 
This TAO implements internal dynamic scheduling.*/
class VecMulDyn : public AssemblyTask 
{
public:   
  
  //! VecMulDyn TAO constructor. 
  /*!
    \param _A is the A vector
    \param _B is the B vector
    \param _C is the Result vector
    \param _len is the length of the vector 
    \param width is the number of resources used by this TAO
    The constructor computes the number of elements per thread and overdecomposes the domain using PSLACK parameter
    In this simple example, we do not instatiate a dynamic scheduler (yet)
  */  
  VecMulDyn(double *_A, double  *_B, double *_C, int _len, 
      int width) : A(_A), B(_B), C(_C), len(_len), AssemblyTask(width) 
  {  
     if(len % (width)) std::cout <<  "Warning: blocklength is not a multiple of TAO width\n";
     blocksize = len / (width*PSLACK); 
     if(!blocksize) std::cout << "Block Length needs to be bigger than " << (width*PSLACK) << std::endl;
     blocks = len / blocksize;
     next = 0;
  }

  //! Inherited pure virtual function that is called by the runtime to cleanup any resources (if any), held by a TAO. 
  void cleanup(){ 
  
  }

  //! Inherited pure virtual function that is called by the runtime upon executing the TAO. 
  /*!
    \param threadid logical thread id that executes the TAO
    This assembly can work totally asynchronously
  */
  void execute(int threadid)
  {
   //  int tid = threadid - leader;
    while(1){
      int blockid = next++;
      if(blockid > blocks) return;
      for(int i = blockid*blocksize; (i < len) && (i < (blockid+1)*blocksize); i++)
          C[i] = A[i] * B[i];
    }
  }

  int blocks;    /*!< TAO implementation specific integer that holds the number of blocks per TAO */
  int blocksize; /*!< TAO implementation specific integer that holds the number of elements per block */
  int len;       /*!< TAO implementation specific integer that holds the vector length */
  double *A;     /*!< TAO implementation specific double array that holds the A vector */
  double *B;     /*!< TAO implementation specific double array that holds the B vector */
  double *C;     /*!< TAO implementation specific double array that holds the result vector */
  atomic<int> next; /*!< TAO implementation specific atomic variable to provide thread safe tracker of the number of processed blocks */
};

#if defined(CRIT_PERF_SCHED)
float VecMulDyn::time_table[XITAO_MAXTHREADS][XITAO_MAXTHREADS]; 
#endif

/*! this TAO will take a set of doubles and add them all together
*/
class VecAdd : public AssemblyTask 
{
public: 
#if defined(CRIT_PERF_SCHED)
  //! A public static class variable.
  /*!
    It holds the performance trace table for the corresponding TAO.
  */      
  static float time_table[][XITAO_MAXTHREADS];
#endif  
  
  //! VecAdd TAO constructor. 
  /*!
    \param _in is the input vector for which the elements should be accumulated
    \param _out is the output element holding the summation     
    \param _len is the length of the vector 
    \param width is the number of resources used by this TAO
  */    
  VecAdd(double *_in, double *_out, int _len, int width) :
        in(_in), out(_out), len(_len), AssemblyTask(width) 
  {  

  }
  //! Inherited pure virtual function that is called by the runtime to cleanup any resources (if any), held by a TAO. 
  void cleanup() {     
  }

  //! Inherited pure virtual function that is called by the runtime upon executing the TAO. 
  /*!
    \param threadid logical thread id that executes the TAO. For this TAO, we let logical core 0 only do the addition to avoid reduction
  */
  void execute(int threadid)
  {
    // let the leader do all the additions, 
    // otherwise we need to code a reduction here, which becomes too ugly
    if(threadid != leader) return;
    *out = 0.0;
    for (int i=0; i < len; i++)
       *out += in[i];
  }
  
  double *in;  /*!< TAO implementation specific double vector that holds the input to be accumulated */
  double *out; /*!< TAO implementation specific double point to the summation*/
  int len;     /*!< TAO implementation specific integer that holds the number of elements */
};
#if defined(CRIT_PERF_SCHED)
float VecAdd::time_table[XITAO_MAXTHREADS][XITAO_MAXTHREADS]; 
#endif

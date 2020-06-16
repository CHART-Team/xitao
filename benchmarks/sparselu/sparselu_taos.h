/*! \file 
@brief Contains the TAOs needed for sparselu
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

using namespace std;

/*! this TAO will take a set of doubles and add them all together
*/
class LU0 : public AssemblyTask 
{
public: 
  //! LU0 TAO constructor. 
  /*!
    \param _in is the input vector for which the elements should be accumulated
    \param _out is the output element holding the summation     
    \param _len is the length of the vector 
    \param width is the number of resources used by this TAO
  */    
  LU0(double *_in, int _len, int width) :
        diag(_in), BSIZE(_len), AssemblyTask(width) 
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
    //std::cout << "LU0 tid: " << threadid << std::endl;
    if(threadid != leader) return;
    int i, j, k;
                                                                                    
    for (k=0; k<BSIZE; k++)
       for (i=k+1; i<BSIZE; i++) {
          diag[i*BSIZE+k] = diag[i*BSIZE+k] / diag[k*BSIZE+k];
          for (j=k+1; j<BSIZE; j++)
             diag[i*BSIZE+j] = diag[i*BSIZE+j] - diag[i*BSIZE+k] * diag[k*BSIZE+j];
      }
  }
  
  double *diag;  /*!< TAO implementation specific double vector that holds the input to be accumulated */
  int BSIZE;     /*!< TAO implementation specific integer that holds the number of elements */
};


/*! this TAO will take a set of doubles and add them all together
*/
class FWD : public AssemblyTask 
{
public: 
  //! FWD TAO constructor. 
  /*!
    \param _in is the input vector for which the elements should be accumulated
    \param _out is the output element holding the summation     
    \param _len is the length of the vector 
    \param width is the number of resources used by this TAO
  */    
  FWD(double *_in, double *_out, int _len, int width) :
        diag(_in), col(_out), BSIZE(_len), AssemblyTask(width) 
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
    //std::cout << "FWD tid: " << threadid << std::endl;
    if(threadid != leader) return;
    int i, j, k;
                                                                               
    for (k=0; k<BSIZE; k++) 
       for (i=k+1; i<BSIZE; i++)
          for (j=0; j<BSIZE; j++)
             col[i*BSIZE+j] = col[i*BSIZE+j] - diag[i*BSIZE+k]*col[k*BSIZE+j];
  }
  
  double *diag;  /*!< TAO implementation specific double vector that holds the input to be accumulated */
  double *col; /*!< TAO implementation specific double point to the summation*/
  int BSIZE;     /*!< TAO implementation specific integer that holds the number of elements */
};

/*! this TAO will take a set of doubles and add them all together
*/
class BDIV : public AssemblyTask 
{
public: 
  //! BDIV TAO constructor. 
  /*!
    \param _in is the input vector for which the elements should be accumulated
    \param _out is the output element holding the summation     
    \param _len is the length of the vector 
    \param width is the number of resources used by this TAO
  */    
  BDIV(double *_in, double *_out, int _len, int width) :
        diag(_in), row(_out), BSIZE(_len), AssemblyTask(width) 
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
    //std::cout << "BDIV tid: " << threadid << std::endl;
    if(threadid != leader) return;
    int i, j, k;
                                                                               
    for (i=0; i<BSIZE; i++)
       for (k=0; k<BSIZE; k++) {
          row[i*BSIZE+k] = row[i*BSIZE+k] / diag[k*BSIZE+k];
          for (j=k+1; j<BSIZE; j++)
             row[i*BSIZE+j] = row[i*BSIZE+j] - row[i*BSIZE+k]*diag[k*BSIZE+j];
       }
    }

  
  double *diag; /*!< TAO implementation specific double point to the summation*/
  double *row;  /*!< TAO implementation specific double vector that holds the input to be accumulated */
  int BSIZE;     /*!< TAO implementation specific integer that holds the number of elements */
};

/*! this TAO will take a set of doubles and add them all together
*/
class BMOD : public AssemblyTask 
{
public: 
  //! BMOD TAO constructor. 
  /*!
    \param _in is the input vector for which the elements should be accumulated
    \param _out is the output element holding the summation     
    \param _len is the length of the vector 
    \param width is the number of resources used by this TAO
  */    
  BMOD(double *_in1, double *_in2,  double *_out, int _len, int width) :
        row(_in1), col(_in2), inner(_out), BSIZE(_len), AssemblyTask(width) 
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
    //std::cout << "BMOD tid: " << threadid << std::endl;
    if(threadid != leader) return;
    int i, j, k;
                                                                                  
    for (i=0; i<BSIZE; i++)
       for (k=0; k<BSIZE; k++) 
          for (j=0; j<BSIZE; j++)
             inner[i*BSIZE+j] = inner[i*BSIZE+j] - row[i*BSIZE+k]*col[k*BSIZE+j];
          
  }
  
  double *row;  /*!< TAO implementation specific double vector that holds the input to be accumulated */
  double *col;  /*!< TAO implementation specific double vector that holds the input to be accumulated */
  double *inner; /*!< TAO implementation specific double point to the summation*/
  int BSIZE;     /*!< TAO implementation specific integer that holds the number of elements */
};

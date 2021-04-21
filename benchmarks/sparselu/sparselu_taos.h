/*! \file 
@brief Contains the TAOs needed for sparselu
*/
#include "config_utils.h"
//#ifdef INIT_TAO
/*! this TAO will take a set of ELEMs and add them all together
*/
class INITB : public AssemblyTask 
{
public: 
  //! LU0 TAO constructor. 
  /*!
    \param _in is the input vector for which the elements should be accumulated
    \param _out is the output element holding the summation     
    \param _len is the length of the vector 
    \param width is the number of resources used by this TAO
  */    
  INITB(ELEM **_in, bool _init, int _len, int width) :
        block(_in), init(_init), BSIZE(_len), AssemblyTask(1)
  {  
    mold = false;
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
    ELEM* p; 
    if(init) {
      int init_val = 1325;
      p = *block; 
      for(i=0; i<BSIZE; ++i) {
        for(j=0; j<BSIZE; ++j) {
          init_val = (3125 * init_val) % 65536;
      	  (*p) = (ELEM)((init_val - 32768.0) / 16384.0);
          p++;
        }
        init_val = 1325;
      }
    } else { 
      p = *block; 
      for(i=0; i<BSIZE; ++i) {
        for(j=0; j<BSIZE; ++j) {
      	  (*p) = 0.0;;
          p++;
        } 
      }
    }
  
  }
  
  bool init;     /*!< TAO implementation specific init parameter that indicates whether to init with values or just with 0 */
  ELEM **block;  /*!< TAO implementation specific ELEM vector that holds the input to be accumulated */
  int BSIZE;     /*!< TAO implementation specific integer that holds the number of elements */
};
//#endif

/*! this TAO will take a set of ELEMs and add them all together
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
  LU0(ELEM *_in, int _len, int width) :
        diag(_in), BSIZE(_len), AssemblyTask(width) 
  {  
    count = 0;
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
   // if(threadid != leader) return;
    int i, j, k = count++;
                                                                                    
    //for (k=0; k<BSIZE; k++)
     while(k < BSIZE) {
       for (i=k+1; i<BSIZE; i++) {
          diag[i*BSIZE+k] = diag[i*BSIZE+k] / diag[k*BSIZE+k];
          for (j=k+1; j<BSIZE; j++)
             diag[i*BSIZE+j] = diag[i*BSIZE+j] - diag[i*BSIZE+k] * diag[k*BSIZE+j];
      }
      k = count++;
     }
  }
  
  ELEM *diag;  /*!< TAO implementation specific ELEM vector that holds the input to be accumulated */
  int BSIZE;     /*!< TAO implementation specific integer that holds the number of elements */
  atomic<int> count;
};


/*! this TAO will take a set of ELEMs and add them all together
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
  FWD(ELEM *_in, ELEM *_out, int _len, int width) :
        diag(_in), col(_out), BSIZE(_len), AssemblyTask(width) 
  {  
    count = 0;
    mold = false;
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
    //if(threadid != leader) return;
    int i, j, k = count++;
                                                                               
    //for (k=0; k<BSIZE; k++) 
     while(k<BSIZE) {
       for (i=k+1; i<BSIZE; i++)
          for (j=0; j<BSIZE; j++)
             col[i*BSIZE+j] = col[i*BSIZE+j] - diag[i*BSIZE+k]*col[k*BSIZE+j];
       k = count++;
     }
  }
  
  ELEM *diag;  /*!< TAO implementation specific ELEM vector that holds the input to be accumulated */
  ELEM *col; /*!< TAO implementation specific ELEM point to the summation*/
  int BSIZE;     /*!< TAO implementation specific integer that holds the number of elements */
  atomic<int> count;
};

/*! this TAO will take a set of ELEMs and add them all together
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
  BDIV(ELEM *_in, ELEM *_out, int _len, int width) :
        diag(_in), row(_out), BSIZE(_len), AssemblyTask(width) 
  {  
    count = 0;
    mold = false;
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
 //   if(threadid != leader) return;
    int i = count++; int j, k;
                                                                               
   // for (i=0; i<BSIZE; i++)
      while(i<BSIZE){
       for (k=0; k<BSIZE; k++) {
          row[i*BSIZE+k] = row[i*BSIZE+k] / diag[k*BSIZE+k];
          for (j=k+1; j<BSIZE; j++)
             row[i*BSIZE+j] = row[i*BSIZE+j] - row[i*BSIZE+k]*diag[k*BSIZE+j];
       }
       i = count++;
      }
    }

  
  ELEM *diag; /*!< TAO implementation specific ELEM point to the summation*/
  ELEM *row;  /*!< TAO implementation specific ELEM vector that holds the input to be accumulated */
  int BSIZE;     /*!< TAO implementation specific integer that holds the number of elements */
  atomic<int> count;
};

/*! this TAO will take a set of ELEMs and add them all together
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
  BMOD(ELEM *_in1, ELEM *_in2,  ELEM **_out, int _len, int width) :
        row(_in1), col(_in2), inner(_out), BSIZE(_len), AssemblyTask(width) 
  {  
    count = 0;
    mold = false;
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
    int i = count++; int j, k;
    ELEM* p = *inner;   
    while(i<BSIZE) {                                                                          
    //for (i=0; i<BSIZE; i++)
       for (k=0; k<BSIZE; k++) 
          for (j=0; j<BSIZE; j++)
             p[i*BSIZE+j] = p[i*BSIZE+j] - row[i*BSIZE+k]*col[k*BSIZE+j];
      i = count++;
    }       
  }
  
  ELEM *row;  /*!< TAO implementation specific ELEM vector that holds the input to be accumulated */
  ELEM *col;  /*!< TAO implementation specific ELEM vector that holds the input to be accumulated */
  ELEM **inner; /*!< TAO implementation specific ELEM point to the summation*/
  int BSIZE;     /*!< TAO implementation specific integer that holds the number of elements */
  atomic<int> count;
};

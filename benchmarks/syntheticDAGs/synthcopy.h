#ifndef SYNTH_COPY
#define SYNTH_COPY

#include "xitao.h"
#include "dtypes.h"
#include <chrono>
#include <iostream>
#include <atomic>
#include <cmath>
#include <stdio.h>
#define PSLACK 8  

// Matrix multiplication, tao groupation on written value
class Synth_MatCopy : public AssemblyTask 
{
public: 

  Synth_MatCopy(uint32_t _size, int _width, real_t *_A, real_t *_B): AssemblyTask(_width), A(_A), B(_B) {   
    dim_size = _size;
    block_size = dim_size / (_width * PSLACK);
    if(block_size == 0) block_size = 1;
    block_index = 0;
    uint32_t elem_count = dim_size * dim_size;
/*    A = new real_t[elem_count]; 
    memset(A, rand(), elem_count*sizeof(real_t)); 
    B = new real_t[elem_count];
    memset(B, rand(), elem_count*sizeof(real_t)); 
*/
    block_count = dim_size / block_size;
  }

  int cleanup(){ 
    //delete[] A;
    //delete[] B;
  }

  // this assembly can work totally asynchronously
  int execute(int threadid) {
    while(true) {
      int row_block_id = block_index++;
      if(row_block_id > block_count) return 0;
      int row_block_start =  row_block_id      * block_size;
      int row_block_end   = (row_block_id + 1) * block_size;
      int end = (dim_size < row_block_end) ? dim_size : row_block_end; 
      for (int i = row_block_start; i < end; ++i) { 
         std::copy(A + (i * dim_size), A + (i * dim_size) + dim_size, B + i * dim_size);
      }
    }
  }

private:
  std::atomic<int> block_index; 
  int block_size; 
  int dim_size;
  int block_count;
  real_t *A, *B;
};


#endif

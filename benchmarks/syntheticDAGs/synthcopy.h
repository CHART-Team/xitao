#ifndef SYNTH_COPY
#define SYNTH_COPY

#include "tao.h"
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
// initialize static parameters
#if defined(CRIT_PERF_SCHED)
  static float time_table[][GOTAO_NTHREADS];
#endif

  Synth_MatCopy(uint32_t _size, int _width): AssemblyTask(_width) {   
    dim_size = _size;
    block_size = dim_size / (_width * PSLACK);
    if(block_size == 0) block_size = 1;
    block_index = 0;
    uint32_t elem_count = dim_size * dim_size;
    A = new real_t[elem_count]; 
    memset(A, rand(), elem_count*sizeof(real_t)); 
    B = new real_t[elem_count];
    memset(B, rand(), elem_count*sizeof(real_t)); 
    block_count = dim_size / block_size;
  }

  int cleanup(){ 
    delete[] A;
    delete[] B;
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

#if defined(CRIT_PERF_SCHED)
  int set_timetable(int threadid, float ticks, int index){
    time_table[index][threadid] = ticks;
  }

  float get_timetable(int threadid, int index){ 
    float time=0;
    time = time_table[index][threadid];
    return time;
  }
#endif
private:
  std::atomic<int> block_index; 
  int block_size; 
  int dim_size;
  int block_count;
  real_t *A, *B;
};


#endif

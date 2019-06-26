#ifndef SYNTH_STENCIL
#define SYNTH_STENCIL

#include "tao.h"
#include "dtypes.h"

#include <chrono>
#include <iostream>
#include <atomic>
#include <cmath>
#include <vector>
#define PSLACK 8  

// Matrix multiplication, tao groupation on written value
class Synth_MatStencil : public AssemblyTask 
{
public: 
// initialize static parameters
#if defined(CRIT_PERF_SCHED)
  static float time_table[][GOTAO_NTHREADS];
#endif

  Synth_MatStencil(uint32_t _size, int _width) : 
  A(_size, std::vector<real_t>(_size)), B(_size, std::vector<real_t>(_size)), AssemblyTask(_width) {   
    dim_size = _size;
    block_size = dim_size / (_width * PSLACK);
    if(block_size == 0) block_size = 1;
    block_index = 0;
    uint32_t elem_count = dim_size * dim_size;
    block_count = dim_size / block_size;
  }

  int cleanup() { 

  }

  int execute(int threadid) {
    while(true) {
      int row_block_id = block_index++;
      if(row_block_id > block_count) return 0;
      int row_block_start =  row_block_id      * block_size;
      int row_block_end   = (row_block_id + 1) * block_size;
      int end = (dim_size < row_block_end) ? dim_size : row_block_end; 
      if (row_block_start == 0) row_block_start = 1;
      if (end == dim_size)      end = dim_size - 1;
      for (int i = row_block_start; i < end; ++i) { 
        for (int j = 1; j < dim_size-1; j++) {
             B[i][j] = A[i][j] + k * (
             A[i-1][j] +
             A[i+1][j] +
             A[i][j-1] +
             A[i][j+1] +
             (-4)*A[i][j] );
        }
      }
    }
  }

#if defined(CRIT_PERF_SCHED)
  int set_timetable(int threadid, float ticks, int index) {
    time_table[index][threadid] = ticks;
  }

  float get_timetable(int threadid, int index) { 
    float time=0;
    time = time_table[index][threadid];
    return time;
  }
#endif
private:
  const real_t k = 0.001;
  std::atomic<int> block_index; 
  int dim_size;
  int block_count;
  int block_size;
  std::vector<std::vector<real_t> > A, B;
};

#endif
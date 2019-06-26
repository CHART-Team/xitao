#ifndef SYNTH_COPY
#define SYNTH_COPY

#include "tao.h"
#include <chrono>
#include <iostream>
#include <atomic>
#include <cmath>

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
    mat_size = _size;
    block_size = mat_size / (_width * PSLACK);
    if(block_size == 0) block_size = 1;
    block_index = 0;
    uint32_t elem_count = mat_size * mat_size;
    A = new double[elem_count]; 
    B = new double[elem_count];
    block_count = mat_size / block_size;
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
      int block_start =  row_block_id      * block_size;
      int block_end   = (row_block_id + 1) * block_size;
      int end = (mat_size < block_end) ? mat_size : block_end; 
      std::copy(A + block_start, A + end, B + block_start);
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
  int mat_size;
  int block_count;
  double *A, *B;
};


#endif
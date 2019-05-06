#include "tao.h"
#include <chrono>
#include <iostream>
#include <atomic>

extern "C" {
#include <stdio.h>
#include <stdlib.h> 
#include <unistd.h>
}

/***********************************************************************
 * main: 
 **********************************************************************/
void fill_arrays(int **a, int **b, int **c);

// Matrix multiplication, tao groupation on written value
class TAO_matrix : public AssemblyTask 
{
public: 
  // initialize static parameters
#if defined(CRIT_PERF_SCHED)
  static float time_table[][GOTAO_NTHREADS];
#endif

  TAO_matrix(int res, //TAO width
             int mini, //start y (used for segmenting the matrix into blocks)
             int maxi, //stop y
             int minj, //start x
             int maxj, //stop x
             int c_offset, //offset to output (used to save output at a different destination)
             int r_size, //row size
             int **m_a, //input matrices
             int **m_c) //output matrix
  : _res(res), imax(maxi), jmin(minj), jmax(maxj), offset(c_offset), AssemblyTask(res) {   

    a = m_a;
    c = m_c;
    i = mini;
    i_lock.lock = false;  
    ROW_SIZE = r_size;    
  }

  int cleanup(){ 
  }

  // this assembly can work totally asynchronously
  int execute(int threadid){
    int temp_j, temp_i;
    while (1){ 
      //mutex for i to make each Processing unit work on one row.
      LOCK_ACQUIRE(i_lock); //Maybe Test-and-set would be better
      temp_i = i++;
      LOCK_RELEASE(i_lock);
      
      if (temp_i >= imax) { // no more work to be done
        break;
      }
      
      //for each column, calculate the output of c[i][j]
      for(temp_j = 0; temp_j < ROW_SIZE; temp_j++){ 
        int temp_output = 0;
        for (int k = 0; k < ROW_SIZE ; k++){ 
          temp_output += (a[temp_i][k] * a[temp_j][k+jmin+ROW_SIZE]);
        }
        c[temp_i][temp_j+offset] = temp_output;
      }    
    }
  }        
  GENERIC_LOCK(i_lock);
  
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
  
  //Variable declaration
  int i, jmin, jmax, imax, _res, ROW_SIZE, offset;
  int **a;
  int **c;
};


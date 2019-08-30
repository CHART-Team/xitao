#include "tao.h"
#include <chrono>
#include <iostream>
#include <atomic>
#include <cmath>

extern "C" {
#include <stdio.h>
#include <stdlib.h> 
#include <unistd.h>
}

/***********************************************************************
 * main: 
 **********************************************************************/
void fill_arrays(int **a, int **b, int **c);

class TAO_Copy : public AssemblyTask 
{
public:
#if defined(CRIT_PERF_SCHED)
  static float time_table[][XITAO_MAXTHREADS];
#endif
  
  TAO_Copy(int *a,  //input
    int *b,       //output
    int s,        //size
    int res)      //assembly width
  : AssemblyTask(res){     
    input = a;
    output = b;
    size = s;
    jobs = 0;
    job_lock.lock = false;
  }

  int cleanup(){ 
  }

  int execute(int threadid)
  {
    int slize = pow(width,2);
    int i;
    while(1){ 
      LOCK_ACQUIRE(job_lock); //Maybe Test-and-set would be better
      i = jobs++;
      LOCK_RELEASE(job_lock);
                      
      if (i >= slize){ // no more work to be don
        break;
      }
      memcpy(&input[i*(size/slize)],&output[i*(size/slize)], (size/slize)*4);
    }
  }
              
  int *input;
  int *output;
  int size;
  int jobs;
  GENERIC_LOCK(job_lock);
     
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
};


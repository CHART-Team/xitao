#include "../tao.h"
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




class TAO_Copy : public AssemblyTask 
{
        public: 


#ifdef  TIME_TRACE             
                static double time_table[][3];
#endif

                TAO_Copy(int *a, //input
                         int *b, //output
                         int s, //size
                         int res) //assembly width
                          : AssemblyTask(res)
                {     
                  input = a;
                  output = b;
                  size = s;

                }

                int cleanup(){ 
                    }


                int execute(int threadid)
                {
                    int virt_id = leader - threadid;
                    int i = (virt_id % width) * (size/width);
                    memcpy(&input[i],&output[i], (size/width)*4);
                }
              
              int *input;
              int *output;
              int size;
#ifdef  TIME_TRACE             
  //            GENERIC_LOCK(ttable_lock);

              int set_timetable(int threadid, double ticks, int index){
    //              LOCK_ACQUIRE(ttable_lock);
                  time_table[threadid][index] = ticks;
      //            LOCK_RELEASE(ttable_lock);
              }

       double get_timetable(int threadid, int index){
     
                  double time=0;
        //          LOCK_ACQUIRE(ttable_lock);
      time = time_table[threadid][index];
  //    LOCK_RELEASE(ttable_lock);
            return time;
       }
#endif    
};


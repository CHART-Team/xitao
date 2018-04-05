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



// Matrix multiplication, tao groupation on written value
class TAO_matrix : public AssemblyTask 
{
        public: 
                // initialize static parameters
#ifdef  TIME_TRACE             
                static double time_table[][3];
#endif
#ifdef  INT_SOL            
                static uint64_t time_table[][3];
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
                        : _res(res), imax(maxi), jmin(minj), jmax(maxj), offset(c_offset), AssemblyTask(res) 
                {   

                  a = m_a;
                  c = m_c;
                  i = mini;
                  ROW_SIZE = r_size;
                
                }

                int cleanup(){ 
                    }

                // this assembly can work totally asynchronously
                int execute(int threadid)
                {

                    int temp_j, temp_i;
                      while (1){ 
                        //mutex for i to make each Processing unit work on one row.
                        LOCK_ACQUIRE(i_lock); //Maybe Test-and-set would be better
                        temp_i = i++;
                        LOCK_RELEASE(i_lock)
                        
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
#ifdef  TIME_TRACE             
              GENERIC_LOCK(ttable_lock);

              int set_timetable(int threadid, double ticks){
                  int index = (_res == 4) ? (2) : ((_res)-1);
                  LOCK_ACQUIRE(ttable_lock);
                  //  time_table[_res][threadid] = (d[_res][threadid] + ticks)/2;
                  time_table[threadid][index] = ticks;
                  LOCK_RELEASE(ttable_lock);
              }
#endif                            
#ifdef  INT_SOL            
              GENERIC_LOCK(ttable_lock);

              int set_timetable(int threadid, uint64_t ticks){
                  int index = (_res == 4) ? (2) : ((_res)-1);
                  LOCK_ACQUIRE(ttable_lock);
                  //  time_table[_res][threadid] = (d[_res][threadid] + ticks)/2;
                  time_table[threadid][index] = ticks;
                  LOCK_RELEASE(ttable_lock);
              }
#endif                            
              //Variable declaration
              int i, jmin, jmax, imax, _res, ROW_SIZE, offset;
              int **a;
              int **c;
};


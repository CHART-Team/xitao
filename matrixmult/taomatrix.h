#include "tao.h"
#include <chrono>
#include <iostream>
#include <atomic>
#include "config-matrix.h"


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
                TAO_matrix(int res, int mini, int maxi, int minj, int maxj, int steps,  int **m_a, int **m_b, int **m_c) 
                        : _res(res), imax(maxi), jmin(minj), jmax(maxj), AssemblyTask(res) 
                {   

                  a = m_a;
                  b = m_b;
                  c = m_c;
                  i = mini;
                
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
                        for(temp_j = jmin; temp_j < jmax; temp_j++){ 
                          int temp_output = 0;
                          for (int k = 0; k < ROW_SIZE ; k++){ 
                            temp_output += (a[temp_i][k] * b[k][temp_j]);
                          }
                          c[temp_i][temp_j] = temp_output;
                        }
                  


                    /* OLD IMPLEMENTATION
                    while(1){

                    
                    int temp_i, temp_j;
                   // if (_res > 1) {
                      LOCK_ACQUIRE(ij_lock);
                        if (i >= imax) {
                          if (++j >= jmax) {
                            stop = 1;
                            LOCK_RELEASE(ij_lock);
                            break;
                          }
                          else {
                            i = imin;
                            temp_i = i;
                            temp_j = j;
                            LOCK_RELEASE(ij_lock);
                          }
                          
                        }
                      else {
                        temp_i = i;
                        temp_j = j;
                        i++;
                        LOCK_RELEASE(ij_lock);
                      }
                    

                   // }

                    int temp_output = 0;
                    for (int k = 0 ; k < ROW_SIZE ; k++){
                      temp_output += (a[temp_i][k] * b[k][temp_j]);
                    }
                    c[temp_i][temp_j] = temp_output;
                    */
                    
                  }
                }
              
              GENERIC_LOCK(i_lock);
              
              //Variable declaration
              int i, jmin, jmax, imax, _res;
              int **a;
              int **b;
              int **c;
};


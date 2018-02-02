/******************************************************************************************/
/*  This program is part of the Barcelona OpenMP Tasks Suite                              */
/*  Copyright (C) 2009 Barcelona Supercomputing Center - Centro Nacional de Supercomputacion  */
/*  Copyright (C) 2009 Universitat Politecnica de Catalunya                               */
/*                                                                                        */
/*  This program is free software; you can redistribute it and/or modify                  */
/*  it under the terms of the GNU General Public License as published by                  */
/*  the Free Software Foundation; either version 2 of the License, or                     */
/*  (at your option) any later version.                                                   */
/*                                                                                        */
/*  This program is distributed in the hope that it will be useful,                       */
/*  but WITHOUT ANY WARRANTY; without even the implied warranty of                        */
/*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                         */
/*  GNU General Public License for more details.                                          */
/*                                                                                        */
/*  You should have received a copy of the GNU General Public License                     */
/*  along with this program; if not, write to the Free Software                           */
/*  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA        */
/******************************************************************************************/

/***********************************************************************
 * main function & common behaviour of the benchmark.
 **********************************************************************/

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




// Matrix multiplication, tao groupation on written value
class TAO_matrix : public AssemblyTask 
{
        public: 
                // initialize static parameters
                TAO_matrix(int res, int mini, int maxi, int minj, int maxj, int steps,  int m_a[COL_SIZE][ROW_SIZE], int m_b[COL_SIZE][ROW_SIZE], int m_c[COL_SIZE][ROW_SIZE]) 
                        : _res(res), imax(maxi), jmin(minj), jmax(maxj), AssemblyTask(res) 
                {   
                  (int (*)[COL_SIZE])a = m_a;
                  (int (*)[COL_SIZE])b = m_b;
                  (int (*)[COL_SIZE])c = m_c;
                  i = mini;
                  j = minj;
                  stop = 0;
                
                }

                int cleanup(){ 
                    }

                // this assembly can work totally asynchronously
                int execute(int threadid)
                {
                  while (stop == 0){
                    if (_res > 1) {
                      LOCK_ACQUIRE(ij_lock);
                        if (++j >= jmax) {
                          if (++i >= imax) {
                            stop = 1;
                            break;
                          }
                          else {
                            j = jmin;
                          }
                        }
                      int temp_i = i;
                      int temp_j = j;
                      LOCK_RELEASE(ij_lock);

                    }
                    else {
                      int temp_i = i;
                      int temp_j = j;
                    }

                    int temp_output = 0;
                    for (int k = 0 ; k < ROW_SIZE ; k++){
                      temp_output += (a[temp_i][k] * b[k][temp_j]);
                    }
                    c[i][j] = temp_output;

                  }
                }

              GENERIC_LOCK(ij_lock);
              int i, j, stop, jmin, jmax, imax, _res;
              int a[COL_SIZE][ROW_SIZE];
              int b[COL_SIZE][ROW_SIZE];
              int c[COL_SIZE][ROW_SIZE];
};


int
main(int argc, char* argv[])
{

   int thread_base; int nthreads; int nctx; //Do we need the last parameter?
   int c_size; int r_size; int stepsize;
   int nqueues;  // how many queues to fill: What is this??
   int nas;      // number of assemblies per queue: what is this??


  
  int a[4][4] = {{1,1,1,1},{1,1,1,1},{1,1,1,1},{1,1,1,1}};
  int b[4][4] = {{1,1,1,1},{1,1,1,1},{1,1,1,1},{1,1,1,1}};
  int c[4][4] = {{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}};



 // set the number of threads and thread_base
/*
   if(getenv("GOTAO_NTHREADS"))
        nthreads = atoi(getenv("GOTAO_NTHREADS"));
   else
        nthreads = GOTAO_NTHREADS;

   if(getenv("GOTAO_THREAD_BASE"))
        thread_base = atoi(getenv("GOTAO_THREAD_BASE"));
   else
        thread_base = GOTAO_THREAD_BASE;

   if(getenv("GOTAO_HW_CONTEXTS"))
        nctx = atoi(getenv("GOTAO_HW_CONTEXTS"));
   else
        nctx = GOTAO_HW_CONTEXTS;
*/
   if(getenv("ROW_SIZE"))
	r_size=atoi(getenv("ROW_SIZE"));
 
   else
	r_size=ROW_SIZE;

   if(getenv("COL_SIZE"))
	c_size=atoi(getenv("COL_SIZE"));
   else
	c_size=COL_SIZE;

   if(getenv("STEP_SIZE"))
	stepsize=atoi(getenv("STEP_SIZE"));
   else
	stepsize=STEP_SIZE;

   if((COL_SIZE != ROW_SIZE) || (0!=(stepsize % COL_SIZE))){
	std::cout << "Incompatible matrix parameters, please choose ROW_SIZE=COL_SIZE and a stepsize that is a divisior of the ROW_SIZE&&COL_SIZE";
	break;
   }


    std::cout << "gotao_init parameters are: " << nthreads <<"," << thread_base << ","<< nctx << std::endl;

   gotao_init();


   std::chrono::time_point<std::chrono::system_clock> start, end;
   start = std::chrono::system_clock::now();


   //We need lock on our output matrix??


   //Spawn TAOs for every seperately written too value, equivilent to for(i->i+stepsize){for(j->j+stepsize) generation}
   int total_assemblies = (c_size/stepsize)*(r_size/stepsize);
   TAO_matrix *ao[total_assemblies];

   //i to keep track of spawned taos
   int i=0;
   	for(int x=0; x<ROW_SIZE; x+=stepsize){
		for(int y=0; y<COL_SIZE; y+=stepsize){
         		ao[i] = new TAO_matrix(1, x, x+stepsize, y, y+stepsize, stepsize, a, i, c); //NOT SURE ABOUT WIDTH  also m_a, m_b not defined but should be matrixes
	  	 	gotao_push_init(ao[i], i % nthreads);
			i++;
		}	
    	}
   
   }
   

   //Do we need to spawn a collector TAO? Should we have output?



   // measure execution time

   goTAO_start();

   goTAO_fini();
   end = std::chrono::system_clock::now();

   std::chrono::duration<double> elapsed_seconds = end-start;
   std::time_t end_time = std::chrono::system_clock::to_time_t(end);
 
   std::cout << "elapsed time: " << elapsed_seconds.count() << "s. " << "Total number of steals: " <<  tao_total_steals << "\n";
   std::cout << "Assembly Throughput: " << total_assemblies / elapsed_seconds.count() << " A/sec\n";
   std::cout << "Assembly Cycle: " << elapsed_seconds.count() / total_assemblies  << " sec/A\n";


  std::cout << c[0][0] << c[0][1] << c[0][2] << c[0][3] << "\n";
  std::cout << c[1][0] << c[1][1] << c[1][2] << c[1][3] << "\n";
  std::cout << c[2][0] << c[2][1] << c[2][2] << c[2][3] << "\n";
  std::cout << c[3][0] << c[3][1] << c[3][2] << c[3][3] << "\n";
   return (0);
}


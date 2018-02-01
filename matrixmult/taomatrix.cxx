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
#include "conifg-matrix.h"

extern "C" {

#include <stdio.h>
#include <stdlib.h> 
#include <unistd.h>

}

/***********************************************************************
 * main: 
 **********************************************************************/

struct matrixmult_ouw{
        //std::list <qmatrixmult_ouw *> out;
        int range_start;
	int range_stop;
	int stpz;
	int input_a[COL_SIZE][ROW_SIZE];
	int input_b[COL_SIZE][ROW_SIZE];
};



// Matrix multiplication, tao groupation on written value
class TAO_matrix : public AssemblyTask 
{
        public: 
                // initialize static parameters
                TAO_matrix(int res, int mini, int maxi, int minj, int maxj, int steps,  int m_a[COL_SIZE][ROW_SIZE], int m_b[COL_SIZE][ROW_SIZE]) 
                        : AssemblyTask(res) 
                {   
			for(int i=0; i<ROW_SIZE; i+=stepsize){
				dow[i].range_start = i;
				dow[i].range_stop = (i+stepsize-1);
				dow[i].stpz = steps;
				dow[i].input_a = m_a; //??
				dow[i].input_b = m_b; //??
				//Since there are no dependencies we should not need an out matrix_mult_ouw ??
				readyq.push_front(&dow[i]);
			}
                }

                int cleanup(){ 
                    }

                // this assembly can work totally asynchronously
                int execute(int threadid)
                {
                }
		matrixmult_ouw dow[MAX_CHUNK]; //How much should this be?

};


int
main(int argc, char* argv[])
{

   int thread_base; int nthreads; int nctx; //Do we need the last parameter?
   int c_size; int r_size; int stepsize;
   int nqueues;  // how many queues to fill: What is this??
   int nas;      // number of assemblies per queue: what is this??



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
         		ao[i] = new TAO_matrix(1, x, x+stepsize, y, y+stepsize, stepsize, m_a, m_b); //NOT SURE ABOUT WIDTH  also m_a, m_b not defined but should be matrixes
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

   return (0);
}


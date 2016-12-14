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

extern "C" {

#include <stdio.h>
#include <stdlib.h> 
#include <unistd.h>

}

/***********************************************************************
 * main: 
 **********************************************************************/

// this is a minimal assembly that does nothing besides allocating workers
class TAO_U : public AssemblyTask 
{
        public: 
                // initialize static parameters
                TAO_U(int width, int nthread=0) 
                        : AssemblyTask(width, nthread) 
                {   

                }

                int cleanup(){ 
                    }

                // this assembly can work totally asynchronously
                int execute(int threadid)
                {
                }

};


int
main(int argc, char* argv[])
{

   int thread_base; int nthreads;
   int nqueues;  // how many queues to fill 
   int awidth;    // width of assemblies
   int nas;      // number of assemblies per queue

   if(argc != 4){
     printf("USage: ./microb <nqueues> <assembly_width> <num_assemblies>\n");
     exit(1);
     }

   nqueues = atoi(argv[1]);
   awidth  = atoi(argv[2]);
   nas     = atoi(argv[3]);

   gotao_init();

   std::chrono::time_point<std::chrono::system_clock> start, end;
   start = std::chrono::system_clock::now();
   // generate workload and insert it into the number of requested queues, in round robin
   int total_assemblies = nqueues * nas;
   TAO_U *ao[total_assemblies];
   for(int i = 0; i < total_assemblies ; i++){
           ao[i] = new TAO_U(awidth);
	   gotao_push_init(ao[i], i % nqueues);
//           worker_ready_q[(i % nqueues) + thread_base].push_back(ao[i]);
   }

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


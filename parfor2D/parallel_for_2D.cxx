/***********************************************************************
 * Example of a 2D pattern to be used by stencil codes
 * The base class itself knows the base address of the input and the
 * number of columns, as well as the base address of the output
 **********************************************************************/

#include "tao.h"
#include "tao_parfor2D.h"

#include <chrono>
#include <iostream>
#include <atomic>
#include <stdlib.h>

// square a matrix using a 2D pattern
class parfor2D_square : public TAO_PAR_FOR_2D_BASE
{
        public: 
                parfor2D_square(void *a, void*c, int rows, int cols, int off1, int off2, int chunk1, int chunk2,
                                gotao_schedule_2D sched,  int chunkx, int chunky, int width, int nthread=0) 
                                    : TAO_PAR_FOR_2D_BASE(a,c,rows,cols,0,0,rows,cols,sched,chunkx,chunky,width,-1) {}

                int compute_for2D(int offx, int offy, int chunkx, int chunky)
                {

                    std::cout << "compute_for2D received parameters: in " << gotao_parfor2D_in << 
                                 " out " << gotao_parfor2D_out << 
                                 " offx " << offx << " offy " << offy << " chunkx " << chunkx << 
                                 " chunky " << chunky <<std::endl;
                    int *mat1 = (int *) gotao_parfor2D_in;
                    int *mat2 = (int *) gotao_parfor2D_out;
                    int cols = gotao_parfor2D_cols;

                    for(int i = offx; i < offx + chunkx; i++)
                      for(int j = offy; j < offy + chunky; j++)
                          mat2[i*cols + j] = mat1[i*cols + j]*mat1[i*cols + j];
                }
};

int worker_loop(int, int, int);

// what this application will do is to multiply two vectors

int
main(int argc, char* argv[])
{
   int nthreads, thread_base;
   int matsize = 0; 
   int chunksize = 0;
   int awidth = 0;

   if(argc != 4){
           std::cout << "./partest <matsize> <chunksize> <assembly width>" << std::endl;
           exit(1);
   }


   matsize = abs(atoi(argv[1]));
   chunksize = abs(atoi(argv[2]));
   awidth = abs(atoi(argv[3]));

   int mat1[matsize][matsize];
   int mat2[matsize][matsize];

   for(int i = 0; i < matsize; i++)
     for(int j = 0; j < matsize; j++)
           mat1[i][j] = (int) random() % 16;

   if(getenv("TAO_NTHREADS"))
        nthreads = atoi(getenv("TAO_NTHREADS"));
   else 
        nthreads = GOTAO_NTHREADS;

   if(getenv("TAO_THREAD_BASE"))
        thread_base = atoi(getenv("TAO_THREAD_BASE"));
   else
        thread_base = GOTAO_THREAD_BASE;

   goTAO_init(nthreads, thread_base);

   parfor2D_square *compute;
   compute = new parfor2D_square(
                     mat1, // pointer to first matrix
                     mat2, // pointer to output matrix
                     matsize, // rows
                     matsize, // columns
                     0, 0,    // offsets
                     matsize, matsize, // chunk rows 
                     gotao_sched_2D_static, // schedule
                     chunksize, // chunk size in rows 
                     chunksize, // chunk size in columns
                     awidth);            // assembly width

   gotao_push_init(compute, 0);

  // bots_t_start = bots_usecs();

   std::chrono::time_point<std::chrono::system_clock> start, end;

   goTAO_start();
   start = std::chrono::system_clock::now();

   goTAO_fini();
   end = std::chrono::system_clock::now();

   std::chrono::duration<double> elapsed_seconds = end-start;
   std::time_t end_time = std::chrono::system_clock::to_time_t(end);
 
   std::cout << "elapsed time: " << elapsed_seconds.count() << "s. " << "Total number of steals: " <<  tao_total_steals << "\n";

   for(int i = 0; i < matsize; i++)
     for(int j = 0; j < matsize; j++)
           std::cout << "entry " << std::dec << i << "," << j <<  " mat1 " << (int) mat1[i][j] << 
                        " mat2 " << (int) mat2[i][j] << std::endl;

   return 0;
}



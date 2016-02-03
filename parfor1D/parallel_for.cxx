/***********************************************************************
 * This is an example for a parallel for pattern that I will develop later
 * into a full blown heat example
 **********************************************************************/
#include "tao.h"
#include "tao_parfor.h"

#include <chrono>
#include <iostream>
#include <atomic>
#include <stdlib.h>


class parfor_mul : public TAO_PAR_FOR_BASE
{
        public: 
                parfor_mul(void *a, void *b, void *c, int elems, gotao_schedule sched, int chunk_size, int width, int nthread=0) : TAO_PAR_FOR_BASE(a,b,c,elems,sched,chunk_size,width) {}

                int compute_for(void *in1, void *in2, void *out3, int off, int chunk)
                {

                    std::cout << "compute_for received parameters: in1 " << in1 << " in2 " << in2 << " out3 " << out3 << " off " << off << " chunk " << chunk << std::endl;
                    int *vec1 = (int *) in1;
                    int *vec2 = (int *) in2;
                    int *vec3 = (int *) out3;

                    for(int i = off; i < off + chunk; i++)
                            vec3[i] = vec1[i] * vec2[i];
                }
};

int worker_loop(int, int, int);

// what this application will do is to multiply two vectors

int
main(int argc, char* argv[])
{
   int nthreads, thread_base;
   int vecsize = 0; 
   int blocksize = 0;
   int chunksize = 0;
   int awidth = 0;

   if(argc != 5){
           std::cout << "./partest <vecsize> <blocksize> <chunksize> <assembly width>" << std::endl;
           exit(1);
   }


   vecsize = abs(atoi(argv[1]));
   blocksize = abs(atoi(argv[2]));
   chunksize = abs(atoi(argv[3]));
   awidth = abs(atoi(argv[4]));

   int vec1[vecsize], vec2[vecsize], vec3[vecsize];

   for(int i = 0; i < vecsize; i++){
           vec1[i] = random() % 16;
           vec2[i] = random() % 16;
   }

   if(getenv("TAO_NTHREADS"))
        nthreads = atoi(getenv("TAO_NTHREADS"));
   else 
        nthreads = NTHREADS;

   if(getenv("TAO_THREAD_BASE"))
        thread_base = atoi(getenv("TAO_THREAD_BASE"));
   else
        thread_base = THREAD_BASE;

   goTAO_init(nthreads, thread_base);

   parfor_mul *parblock[vecsize/blocksize];
   for(int i = 0; i < (vecsize/blocksize); i++){
           parblock[i] = new parfor_mul(
                           vec1+i*blocksize, // pointer to first vector
                           vec2+i*blocksize, // pointer to second vector
                           vec3+i*blocksize, // pointer to third vector
                           blocksize,        // number of elements to process
                           gotao_sched_static, // scheduling policy
                           chunksize,          // chunk size
                           awidth);            // assembly width

           gotao_push_init(parblock[i], 0);
   }

  // bots_t_start = bots_usecs();

   std::chrono::time_point<std::chrono::system_clock> start, end;

   goTAO_start();
   start = std::chrono::system_clock::now();

   goTAO_fini();
   end = std::chrono::system_clock::now();

   std::chrono::duration<double> elapsed_seconds = end-start;
   std::time_t end_time = std::chrono::system_clock::to_time_t(end);
 
   std::cout << "elapsed time: " << elapsed_seconds.count() << "s. " << "Total number of steals: " <<  tao_total_steals << "\n";

   for(int i = 0; i < vecsize; i++)
           std::cout << "entry " << std::dec << i << " vec1 " << (int) vec1[i] << 
                        " vec2 " << (int) vec2[i] << " vec3 " << (int) vec3[i] << std::endl;

   return 0;
}



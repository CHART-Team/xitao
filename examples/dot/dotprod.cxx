
#include "tao.h"
#include <chrono>
#include <iostream>
#include <atomic>

extern "C" {

#include <stdio.h>
#include <stdlib.h> 
#include <unistd.h>

}

using namespace std;

// this TAO will take two vectors and multiply them
// This TAO implements a static schedule
class VecMulSta : public AssemblyTask 
{
        public: 
                // initialize static parameters
                VecMulSta(double *_A, double  *_B, double *_C, int _len, 
                      int width) : A(_A), B(_B), C(_C), len(_len), AssemblyTask(width) 
                {   
                    // compute the number of elements per thread
                    // in this simple example we do not instatiate a dynamic scheduler (yet)

                    if(len % width) std::cout <<  "Warning: blocklength is not a multiple of TAO width\n";
                    block = len / width;

                }

                int cleanup(){ 
                    }

                // this assembly can work totally asynchronously
                int execute(int threadid)
                {
                   int tid = threadid - leader; 
                   for(int i = tid*block; (i < len)  && (i < (tid+1)*block); i++)
                           C[i] = A[i] * B[i];
                }

                // parameters for this TAO
                int block;
                int len;
                double *A, *B, *C;

};


class VecMulDyn : public AssemblyTask 
{
        public: 
                // initialize static parameters
                VecMulDyn(double *_A, double  *_B, double *_C, int _len, 
                      int width) : A(_A), B(_B), C(_C), len(_len), AssemblyTask(width) 
                {   
                    // compute the number of elements per thread
                    // in this simple example we do not instatiate a dynamic scheduler (yet)

// parallel slackness                        
#define PSLACK 8   

                    if(len % (width)) std::cout <<  "Warning: blocklength is not a multiple of TAO width\n";
                    blocksize = len / (width*PSLACK); 
                    if(!blocksize) std::cout << "Block Length needs to be bigger than " << (width*PSLACK) << std::endl;
                    blocks = len / blocksize;
                    next = 0;

                }

                int cleanup(){ 
                    }

                // this assembly can work totally asynchronously
                int execute(int threadid)
                {
                 //  int tid = threadid - leader;
                   while(1){
                     int blockid = next++;
                     if(blockid > blocks) return 0;

                     for(int i = blockid*blocksize; (i < len) && (i < (blockid+1)*blocksize); i++)
                           C[i] = A[i] * B[i];
                   }
                }

                // parameters for this TAO
                int blocksize, blocks;
                int len;
                double *A, *B, *C;
                atomic<int> next;

};


// this TAO will take a set of doubles and add them all together
class VecAdd : public AssemblyTask 
{
        public: 
                VecAdd(double *_in, double *_out, int _len, int width) :
                        in(_in), out(_out), len(_len), AssemblyTask(width) 
                {   

                }

                int cleanup(){ 
                    }

                int execute(int threadid)
                {
                   // let the leader do all the additions, 
                   // otherwise we need to code a reduction here, which becomes too ugly
                   if(threadid != leader) return 0;

                   *out = 0.0;
                   for (int i=0; i < len; i++)
                       *out += in[i];
                }

                // parameters for this TAO
                double *in, *out;
                int len;

};


// we run the example as ./a.out <len> <width> <block>
// where: len   := length of vector
//        width := width of TAOs
//        block := how many elements to process per TAO

int
main(int argc, char *argv[])
{
   double *A, *B, *C, D; 
   if(argc != 4){
      std::cout << "./a.out <veclength> <TAOwidth> <blocklength>" << std::endl; 
      return 0;
      }

   int len = atoi(argv[1]);
   int width = atoi(argv[2]);
   int block = atoi(argv[3]);

   // for simplicity support only perfect partitions
   if(len % block){
      std::cout << "len is not a multiple of block!" << std::endl;
      return 0;
      }


   // no topologies in this version
   A = new double[len];
   B = new double[len];
   C = new double[len];

   // initialize the vectors with some numbers
   for(int i = 0; i < len; i++){
      A[i] = (double) (i+1);
      B[i] = (double) (i+1);
   }

   gotao_init();

   int numvm = len / block;
#ifdef STATIC
   VecMulSta *vm[numvm];
#else
   VecMulDyn *vm[numvm];
#endif
   VecAdd *va = new VecAdd(C, &D, len, width);
   
   for(int j = 0; j < numvm; j++){
#ifdef STATIC
     vm[j] = new VecMulSta(A+j*block, B+j*block, C+j*block, block, width);
#else
     vm[j] = new VecMulDyn(A+j*block, B+j*block, C+j*block, block, width);
#endif
     vm[j]->make_edge(va);
     gotao_push_init(vm[j], j % gotao_nthreads);
   } 

   gotao_start();
   gotao_fini();

   std::cout << "Result is " << D << std::endl;
   std::cout << "Done!\n";
   std::cout << "Total successful steals: " << tao_total_steals << std::endl;
}

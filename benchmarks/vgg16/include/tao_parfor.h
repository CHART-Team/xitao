// tao_parfor.h
// parallel for pattern v1

#pragma once
#include "tao.h"

enum gotao_schedule{
           gotao_sched_static, 
           gotao_sched_dynamic,
};

#define MAX_PARFOR_SIZE 64

#define PARFOR_IDLE 0
#define PARFOR_COMPUTE 1

struct gotao_parfor_af{
        int type; // idle or compute
        int off;  // offsets into vectors
        int chunk_size; // how many elements to process
};

class TAO_PAR_FOR_BASE : public AssemblyTask 
{
        public: 
                TAO_PAR_FOR_BASE(void *a, void *b, void *c, int _elems, gotao_schedule _sched, int _chunk_size, int width, int nthread=0)
                        : AssemblyTask(width, nthread) 
                {   
                       in1 = a; 
                       in2 = b;
                       out3 = c;
                       elems = _elems;
                       sched = _sched;
                       chunk_size = _chunk_size;

                       if(!chunk_size) 
                                chunk_size = elems / width;

                       // assume for now only a static schedule of default chunk elems/width
                       if(sched == gotao_sched_dynamic) 
                               std::cout << "Error: parfor_dynamic not yet implemented\n"
                                            "Proceed using static\n";

                       steps = (((elems+chunk_size -1 )/chunk_size) + width - 1)  / width;

                       for(int thrd = 0; thrd < width; thrd++){
                          for(int st = 0; st < steps; st++){
                              int index = thrd*steps + st;
                              if(index < ((elems+chunk_size-1)/chunk_size)){
                                dow[index].type = PARFOR_COMPUTE;  
                                dow[index].off = index*chunk_size;
                                if((index+1)*chunk_size > elems)
                                  dow[index].chunk_size = elems -  index*chunk_size; 
                                else
                                  dow[index].chunk_size = chunk_size;
                                }
                              else 
                                dow[index].type = PARFOR_IDLE;
                            }
                          }

                }

                int cleanup(){ 
                    }

                void *in1, *in2, *out3;
                int elems; 
                gotao_schedule sched;
                int chunk_size;

                // this function actually performs the computation
                virtual int compute_for(void *, void *, void *, int off, int chunk)=0;

                // this assembly can work totally asynchronously
                int execute(int threadid)
                {
                int tid = threadid - leader;

                for(int s = 0; s < steps; s++){
                   gotao_parfor_af *unit = &dow[tid*steps + s];
                   switch (unit->type){
                           case PARFOR_COMPUTE: 
                                   compute_for(in1, in2, out3, unit->off, unit->chunk_size);
                           case PARFOR_IDLE: break; // do nothing
                      }
                   }
                   return 0; 
                }

                int steps;
                gotao_parfor_af dow[MAX_PARFOR_SIZE]; 
};


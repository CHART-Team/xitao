// tao_parfor2D.h
// this is a pattern to be used with stencils or one to one matrix transformation

#pragma once
#include "tao.h"

enum gotao_schedule_2D{
           gotao_sched_2D_static, 
           gotao_sched_2D_dynamic,
};

#define MAX_PARFOR_SIZE 64

#define PARFOR2D_IDLE 0
#define PARFOR2D_COMPUTE 1

struct gotao_parfor2D_af{
        int type; // compute (this pattern has no idle state, i.e. ychunks multiple of awidth)
        int offx, offy;  // offsets into vectors
        int chunkx, chunky; // how many elements to process in x and y
};

class TAO_PAR_FOR_2D_BASE : public AssemblyTask 
{
        public: 
                TAO_PAR_FOR_2D_BASE(
                                 void *a,     // start of global input matrix
                                 void *b,     // start of global output matrix
                                 int _rows,   // by convention: x axis
                                 int _cols,   // by convention: y axis
                                 int _offx,   // offset in rows 
                                 int _offy,   // offset in columns
                                 int _chunkx, // rows of this assembly
                                 int _chunky, // columns of this assembly
                                 gotao_schedule_2D _sched,  // internal scheduler
                                 int _chunk_size_x,         // internal chunking in x
                                 int _chunk_size_y,         // internal chunking in y
                                 int width,   // assembly width
                                 int nthread=0)
                        : AssemblyTask(width, nthread) 
                {   
                       int xchunks; 
                       int ychunks;

                       gotao_parfor2D_in = a; 
                       gotao_parfor2D_out = b;
                       gotao_parfor2D_rows = _rows;
                       gotao_parfor2D_cols = _cols;
                       gotao_parfor2D_offx = _offx;
                       gotao_parfor2D_offy = _offy;
                       gotao_parfor2D_chunkx = _chunkx;
                       gotao_parfor2D_chunky = _chunky;

                       sched = _sched;
                       internal_chunk_size_x = _chunk_size_x;
                       internal_chunk_size_y = _chunk_size_y;

                       if(!internal_chunk_size_x) 
                                internal_chunk_size_x = gotao_parfor2D_chunkx / width;

                       if(!internal_chunk_size_y) 
                                internal_chunk_size_y = gotao_parfor2D_chunky / width;

                       xchunks = (gotao_parfor2D_chunkx + 
                                       internal_chunk_size_x - 1)/internal_chunk_size_x;
                       ychunks = (gotao_parfor2D_chunky + 
                                       internal_chunk_size_y - 1)/internal_chunk_size_y;

                       // assume for now only a static schedule of default chunk elems/width
                       if(sched == gotao_sched_2D_dynamic) 
                               std::cout << "Error: gotao_sched_2D_dynamic not yet implemented\n"
                                            "Proceed using static\n";

                       // the total number of steps to be completed by this assembly
                       // is the number of 2D chunks divided by the width
                       steps = (xchunks*ychunks + width - 1)/width;

                       for(int thrd = 0; thrd < width; thrd++){
                          for(int st = 0; st < steps; st++){
                              int index = thrd*steps + st; 
                              int index_x = index%xchunks;
                              int index_y = index/xchunks;
                              dow[index].type = PARFOR2D_COMPUTE;  
                              dow[index].offx = gotao_parfor2D_offx + index_x*internal_chunk_size_x;
                              dow[index].offy = gotao_parfor2D_offy + index_y*internal_chunk_size_y;
                              dow[index].chunkx = internal_chunk_size_x;
                              dow[index].chunky = internal_chunk_size_y;
                              }
                            }
                      }


                // the following are global parameters and need to be visible
                void *gotao_parfor2D_in; 
                void *gotao_parfor2D_out;
                int gotao_parfor2D_rows; 
                int gotao_parfor2D_cols; 
                int gotao_parfor2D_offx;
                int gotao_parfor2D_offy;
                int gotao_parfor2D_chunkx;
                int gotao_parfor2D_chunky;

                // these are the parameters that govern the internal partitioning
                gotao_schedule_2D sched;
                int internal_chunk_size_x;
                int internal_chunk_size_y;

                int cleanup(){ 
                    }


                // this function actually performs the computation
                virtual int compute_for2D(int off_x, int off_y, int chunk_x, int chunk_y) = 0;

                int execute(int threadid)
                {
                int tid = threadid - leader;

                for(int s = 0; s < steps; s++){
                   gotao_parfor2D_af *unit = &dow[tid*steps + s];
                   compute_for2D(
                               unit->offx, 
                               unit->offy, 
                               unit->chunkx, 
                               unit->chunky);
                   }
                   return 0; 
                }

                int steps;
                gotao_parfor2D_af dow[MAX_PARFOR_SIZE]; 
};

// taosort.h

#include "../tao.h"
#include <chrono>
#include <iostream>

// the following code is based on the cilksort version distributed by BSC
// I removed the copyright notice for the while

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <atomic>

#define SORT_BUFFER_SIZE (32*1024*1024)
#define MERGE_TASK_SIZE (2*1024)
#define SORT_TASK_SIZE (2*1024)
#define INSERTION_THR (20)

// hereby you define what barrier you want to use
//#define BARRIER cxx_barrier
#define BARRIER spin_barrier


#ifdef DO_LOI
#include "loi.h"

// keep exact same format as LOI example 
enum kernels{
    MERGE = 0, 
    QSORT = 1, // this is libc qsort(), it does not necessarily implement quicksort
    };

/* this structure describes the relationship between phases and kernels in the application */ 
extern struct loi_kernel_info sort_kernels;

#endif
typedef int ELM;

void seqquick(ELM *low, ELM *high);
void seqmerge(ELM *low1, ELM *high1, ELM *low2, ELM *high2, ELM *lowdest);
ELM *binsplit(ELM val, ELM *low, ELM *high);
void cilkmerge(ELM *low1, ELM *high1, ELM *low2, ELM *high2, ELM *lowdest);
void cilksort(ELM *low, ELM *tmp, long size);
void scramble_array( void );
void fill_array( void );
void sort ( void );
void sort_par ( void );
void sort_init ( void );
int  sort_verify ( void );
void print_array( int );

void quicksort(int tid, struct seqquick_uow *arg);
void mergesort(int tid, struct seqmerge_uow *arg);
void binpart(int tid, struct binsplit_uow *arg);
struct idle_uow{ };

extern int sort_buffer_size;
extern int insertion_thr;
extern ELM *array, *tmp, *array1;

//void idle(){};

// from this part we will make the description of work
// and also generate the main() function

#define SEQQUICK 0
#define SEQMERGE 1 
#define BINSPLIT 2
#define IDLE     3

// unit of work descriptors contain static and dynamic parameters
// TODO: the following is quite ugly and unintelligible. Need to propose some way to automatize this
// OTOH, I never intented this to target productivity programmers, but compilers and library

struct seqquick_uow{
        ELM *low; 
        ELM *high;
};

// lower seqmerge - static
struct seqmerge_uow{
        ELM *low1; 
        ELM *high1;
        ELM *low2;
        ELM *high2;
        ELM *lowdest;
};

struct binsplit_uow{
        ELM *regA;  // start of first region 
        ELM *regB;  // start of second region
        int size;   // size of both regions together
        ELM *target; // where to write the output

        ELM **out_low_high2; // used to complete these two structures
        ELM **out_up_low2; // used to complete these two structures
        ELM **out_up_lowdest; // used to complete these two structures
};




struct quickmerge_uow{
        int type; // SEQMERGE, SEQQUICK, BINSPLIT, IDLE
        union{
            struct seqmerge_uow ms;
            struct seqquick_uow sq;
            struct binsplit_uow bs;
            struct idle_uow     id;
        }u;
};
        
void print_array( int, ELM * );


class TAOinit : public AssemblyTask
{
    public: 
               TAOinit(ELM *i, ELM *o, int s, int w) : 
                       AssemblyTask(w), size(s), in(i), out(o) {};
               
               int cleanup(){ 
                    }

               int execute(int threadid)
               { 
               int tid = threadid - leader;
               if(!tid){ // only the leader will copy
                 for(int i = 0; i < size; i++) 
                   out[i] = in[i];
                   }
               }
            
                int size;
                ELM *in, *out;
};
     
class TAOQuickMerge4 : public AssemblyTask 
{
        public: 
                // initialize static parameters
                TAOQuickMerge4(ELM *array, ELM *tmp, int size, int unused=0) : AssemblyTask(4) 
                {   
                       int quarter = size / 4;
                       steps = 5;

                       // STEP 0
                       //
                       dow[0][0].type = SEQQUICK;
                       dow[0][0].u.sq.low  = array + 0*quarter;
                       dow[0][0].u.sq.high = array + 1*quarter - 1;

                       dow[1][0].type = SEQQUICK;
                       dow[1][0].u.sq.low  = array + 1*quarter;
                       dow[1][0].u.sq.high = array + 2*quarter - 1;

                       dow[2][0].type = SEQQUICK;
                       dow[2][0].u.sq.low  = array + 2*quarter;
                       dow[2][0].u.sq.high = array + 3*quarter - 1;

                       dow[3][0].type = SEQQUICK;
                       dow[3][0].u.sq.low  = array + 3*quarter;
                       dow[3][0].u.sq.high = array + size - 1;

                       // STEP 1
                       dow[0][1].type = BINSPLIT; 
                       dow[0][1].u.bs.regA = array;
                       dow[0][1].u.bs.regB = array + quarter;
                       dow[0][1].u.bs.size = quarter*2;
                       dow[0][1].u.bs.target = tmp;
                       // internal storage transfer
                       dow[0][1].u.bs.out_low_high2  =  &dow[0][2].u.ms.high2;
                       dow[0][1].u.bs.out_up_low2    =  &dow[1][2].u.ms.low2;
                       dow[0][1].u.bs.out_up_lowdest =  &dow[1][2].u.ms.lowdest;

                       dow[1][1].type = IDLE;

                       dow[2][1].type = BINSPLIT; 
                       dow[2][1].u.bs.regA    = array + 2*quarter;
                       dow[2][1].u.bs.regB    = array + 3*quarter;
                       dow[2][1].u.bs.size    = size - quarter*2;
                       dow[2][1].u.bs.target  = tmp + 2*quarter;
                       // internal storage transfer
                       dow[2][1].u.bs.out_low_high2  =  &dow[2][2].u.ms.high2;
                       dow[2][1].u.bs.out_up_low2    =  &dow[3][2].u.ms.low2;
                       dow[2][1].u.bs.out_up_lowdest =  &dow[3][2].u.ms.lowdest;

                       dow[3][1].type = IDLE;

                       // STEP 2
                       dow[0][2].type = SEQMERGE;
                       dow[0][2].u.ms.low1   = array;
                       dow[0][2].u.ms.high1  = array + quarter/2 - 1;
                       dow[0][2].u.ms.low2   = array + quarter;
                       //dow[0][2].u.ms.high2  = nullptr;
                       dow[0][2].u.ms.lowdest= tmp;

                       dow[1][2].type = SEQMERGE;
                       dow[1][2].u.ms.low1   = array + quarter/2 + 1;
                       dow[1][2].u.ms.high1  = array + quarter - 1;
                       //dow[1][2].u.ms.low2   = nullptr;
                       dow[1][2].u.ms.high2  = array + quarter*2 -1;
                       //dow[1][2].u.ms.lowdest= nullptr;

                       dow[2][2].type = SEQMERGE;
                       dow[2][2].u.ms.low1   = quarter*2 + array;
                       dow[2][2].u.ms.high1  = quarter*2 + array + quarter/2 - 1;
                       dow[2][2].u.ms.low2   = quarter*2 + array + quarter;
                       //dow[2][2].u.ms.high2  = nullptr;
                       dow[2][2].u.ms.lowdest= quarter*2 + tmp;

                       dow[3][2].type = SEQMERGE;
                       dow[3][2].u.ms.low1   = quarter*2 + array + quarter / 2 + 1;
                       dow[3][2].u.ms.high1  = quarter*2 + array + quarter - 1;
                       //dow[3][2].u.ms.low2   = nullptr;
                       dow[3][2].u.ms.high2  = size + array - 1;
                       //dow[3][2].u.ms.lowdest= nullptr;


                       // STEP 3
                       dow[0][3].type = BINSPLIT; 
                       dow[0][3].u.bs.regA = tmp;
                       dow[0][3].u.bs.regB = tmp + 2*quarter;
                       dow[0][3].u.bs.size = size;
                       dow[0][3].u.bs.target = array;
                       // internal storage transfer
                       dow[0][3].u.bs.out_low_high2  =  &dow[0][4].u.ms.high2;
                       dow[0][3].u.bs.out_up_low2    =  &dow[2][4].u.ms.low2;
                       dow[0][3].u.bs.out_up_lowdest =  &dow[2][4].u.ms.lowdest;

                       dow[1][3].type = IDLE;
                       dow[2][3].type = IDLE;
                       dow[3][3].type = IDLE;

                       // STEP 4
                       dow[0][4].type = SEQMERGE;
                       dow[0][4].u.ms.low1   = tmp;
                       dow[0][4].u.ms.high1  = tmp + quarter - 1;
                       dow[0][4].u.ms.low2   = tmp + 2*quarter;
                       //dow[0][4].u.ms.high2  = nullptr;
                       dow[0][4].u.ms.lowdest= array;

                       dow[2][4].type = SEQMERGE;
                       dow[2][4].u.ms.low1   = tmp + quarter + 1;
                       dow[2][4].u.ms.high1  = tmp + 2*quarter - 1;
                       //dow[1][2].u.ms.low2   = nullptr;
                       dow[2][4].u.ms.high2  = tmp + size -1;
                       //dow[1][2].u.ms.lowdest= nullptr;
                       //
                       dow[1][4].type = IDLE;
                       dow[3][4].type = IDLE;

                }

                int cleanup(){ 
                    }

                int execute(int threadid)
                {
                int tid = threadid - leader;

                for(int s = 0; s < steps; s++){
                   quickmerge_uow *unit = &dow[tid][s];
                   switch (unit->type){
                      case SEQQUICK: quicksort(threadid, &unit->u.sq); break;
                      case SEQMERGE: mergesort(threadid, &unit->u.ms); break;
                      case BINSPLIT:   binpart(threadid, &unit->u.bs);   break;
                      case IDLE: break; // do nothing
                      }
                   barrier->wait();
                   }
                }
            
                int steps;
                quickmerge_uow dow[4][5]; // [threadid][step]
};


// this is exactly the same as TAOQuickMerge except for the missing quicksort in the first step
// As such the utilization is about 75% (compared with 83% in QM4)
class TAOMerge4 : public AssemblyTask 
{
        public: 
                // initialize static parameters
                TAOMerge4(ELM *array, ELM *tmp, int size, int unused=0) : AssemblyTask(4) 
                {   
                       int quarter = size / 4;
                       steps = 4;

                       // STEP 0
                       dow[0][0].type = BINSPLIT; 
                       dow[0][0].u.bs.regA = array;
                       dow[0][0].u.bs.regB = array + quarter;
                       dow[0][0].u.bs.size = quarter*2;
                       dow[0][0].u.bs.target = tmp;
                       // internal storage transfer
                       dow[0][0].u.bs.out_low_high2  =  &dow[0][1].u.ms.high2;
                       dow[0][0].u.bs.out_up_low2    =  &dow[1][1].u.ms.low2;
                       dow[0][0].u.bs.out_up_lowdest =  &dow[1][1].u.ms.lowdest;

                       dow[1][0].type = IDLE;

                       dow[2][0].type = BINSPLIT; 
                       dow[2][0].u.bs.regA    = array + 2*quarter;
                       dow[2][0].u.bs.regB    = array + 3*quarter;
                       dow[2][0].u.bs.size    = size - quarter*2;
                       dow[2][0].u.bs.target  = tmp + 2*quarter;
                       // internal storage transfer
                       dow[2][0].u.bs.out_low_high2  =  &dow[2][1].u.ms.high2;
                       dow[2][0].u.bs.out_up_low2    =  &dow[3][1].u.ms.low2;
                       dow[2][0].u.bs.out_up_lowdest =  &dow[3][1].u.ms.lowdest;

                       dow[3][0].type = IDLE;

                       // STEP 1
                       dow[0][1].type = SEQMERGE;
                       dow[0][1].u.ms.low1   = array;
                       dow[0][1].u.ms.high1  = array + quarter/2 - 1;
                       dow[0][1].u.ms.low2   = array + quarter;
                       //dow[0][2].u.ms.high2  = nullptr;
                       dow[0][1].u.ms.lowdest= tmp;

                       dow[1][1].type = SEQMERGE;
                       dow[1][1].u.ms.low1   = array + quarter/2 + 1;
                       dow[1][1].u.ms.high1  = array + quarter - 1;
                       //dow[1][2].u.ms.low2   = nullptr;
                       dow[1][1].u.ms.high2  = array + quarter*2 -1;
                       //dow[1][2].u.ms.lowdest= nullptr;

                       dow[2][1].type = SEQMERGE;
                       dow[2][1].u.ms.low1   = quarter*2 + array;
                       dow[2][1].u.ms.high1  = quarter*2 + array + quarter/2 - 1;
                       dow[2][1].u.ms.low2   = quarter*2 + array + quarter;
                       //dow[2][2].u.ms.high2  = nullptr;
                       dow[2][1].u.ms.lowdest= quarter*2 + tmp;

                       dow[3][1].type = SEQMERGE;
                       dow[3][1].u.ms.low1   = quarter*2 + array + quarter / 2 + 1;
                       dow[3][1].u.ms.high1  = quarter*2 + array + quarter - 1;
                       //dow[3][2].u.ms.low2   = nullptr;
                       dow[3][1].u.ms.high2  = size + array - 1;
                       //dow[3][2].u.ms.lowdest= nullptr;


                       // STEP 2
                       dow[0][2].type = BINSPLIT; 
                       dow[0][2].u.bs.regA = tmp;
                       dow[0][2].u.bs.regB = tmp + 2*quarter;
                       dow[0][2].u.bs.size = size;
                       dow[0][2].u.bs.target = array;
                       // internal storage transfer
                       dow[0][2].u.bs.out_low_high2  =  &dow[0][3].u.ms.high2;
                       dow[0][2].u.bs.out_up_low2    =  &dow[2][3].u.ms.low2;
                       dow[0][2].u.bs.out_up_lowdest =  &dow[2][3].u.ms.lowdest;

                       dow[1][2].type = IDLE;
                       dow[2][2].type = IDLE;
                       dow[3][2].type = IDLE;


                       // STEP 3
                       dow[0][3].type = SEQMERGE;
                       dow[0][3].u.ms.low1   = tmp;
                       dow[0][3].u.ms.high1  = tmp + quarter - 1;
                       dow[0][3].u.ms.low2   = tmp + 2*quarter;
                       //dow[0][4].u.ms.high2  = nullptr;
                       dow[0][3].u.ms.lowdest= array;

                       dow[2][3].type = SEQMERGE;
                       dow[2][3].u.ms.low1   = tmp + quarter + 1;
                       dow[2][3].u.ms.high1  = tmp + 2*quarter - 1;
                       //dow[1][2].u.ms.low2   = nullptr;
                       dow[2][3].u.ms.high2  = tmp + size -1;
                       //dow[1][2].u.ms.lowdest= nullptr;
                       //
                       dow[1][3].type = IDLE;
                       dow[3][3].type = IDLE;

                }

                int cleanup(){ 
                    }

                int execute(int threadid)
                {
                int tid = threadid - leader;

                for(int s = 0; s < steps; s++){
                   quickmerge_uow *unit = &dow[tid][s];
                   switch (unit->type){
                      case SEQMERGE: mergesort(threadid, &unit->u.ms); break;
                      case BINSPLIT:   binpart(threadid, &unit->u.bs); break;
                      case IDLE: break; // do nothing
                      }
                   barrier->wait();
                   }
                }
            
                int steps;
                quickmerge_uow dow[4][4]; // [threadid][step]
};

class TAOQuickMerge2 : public AssemblyTask 
{
        public: 
                // initialize static parameters
                TAOQuickMerge2(ELM *array, ELM *tmp, int size, int unused=0) : AssemblyTask(2) 
                {   
                       int quarter = size / 4;
                       steps = 7;
                       // STEP 0
                       //
                       dow[0][0].type = SEQQUICK;
                       dow[0][0].u.sq.low  = array + 0*quarter;
                       dow[0][0].u.sq.high = array + 1*quarter - 1;

                       dow[1][0].type = SEQQUICK;
                       dow[1][0].u.sq.low  = array + 1*quarter;
                       dow[1][0].u.sq.high = array + 2*quarter - 1;

                       // STEP1
                       dow[0][1].type = SEQQUICK;
                       dow[0][1].u.sq.low  = array + 2*quarter;
                       dow[0][1].u.sq.high = array + 3*quarter - 1;

                       dow[1][1].type = SEQQUICK;
                       dow[1][1].u.sq.low  = array + 3*quarter;
                       dow[1][1].u.sq.high = array + size - 1;

                       // STEP 2
                       dow[0][2].type = BINSPLIT; 
                       dow[0][2].u.bs.regA = array;
                       dow[0][2].u.bs.regB = array + quarter;
                       dow[0][2].u.bs.size = quarter*2;
                       dow[0][2].u.bs.target = tmp;
                       dow[0][2].u.bs.out_low_high2  =  &dow[0][3].u.ms.high2;
                       dow[0][2].u.bs.out_up_low2    =  &dow[1][3].u.ms.low2;
                       dow[0][2].u.bs.out_up_lowdest =  &dow[1][3].u.ms.lowdest;

                       dow[1][2].type = BINSPLIT; 
                       dow[1][2].u.bs.regA    = array + 2*quarter;
                       dow[1][2].u.bs.regB    = array + 3*quarter;
                       dow[1][2].u.bs.size    = size - quarter*2;
                       dow[1][2].u.bs.target  = tmp + 2*quarter;
                       dow[1][2].u.bs.out_low_high2  =  &dow[0][4].u.ms.high2;
                       dow[1][2].u.bs.out_up_low2    =  &dow[1][4].u.ms.low2;
                       dow[1][2].u.bs.out_up_lowdest =  &dow[1][4].u.ms.lowdest;

                       // STEP 3
                       dow[0][3].type = SEQMERGE;
                       dow[0][3].u.ms.low1   = array;
                       dow[0][3].u.ms.high1  = array + quarter/2 - 1;
                       dow[0][3].u.ms.low2   = array + quarter;
                       dow[0][3].u.ms.lowdest= tmp;

                       dow[1][3].type = SEQMERGE;
                       dow[1][3].u.ms.low1   = array + quarter/2 + 1;
                       dow[1][3].u.ms.high1  = array + quarter - 1;
                       dow[1][3].u.ms.high2  = array + quarter*2 -1;


                       // STEP 4
                       dow[0][4].type = SEQMERGE;
                       dow[0][4].u.ms.low1   = quarter*2 + array;
                       dow[0][4].u.ms.high1  = quarter*2 + array + quarter/2 - 1;
                       dow[0][4].u.ms.low2   = quarter*2 + array + quarter;
                       dow[0][4].u.ms.lowdest= quarter*2 + tmp;

                       dow[1][4].type = SEQMERGE;
                       dow[1][4].u.ms.low1   = quarter*2 + array + quarter / 2 + 1;
                       dow[1][4].u.ms.high1  = quarter*2 + array + quarter - 1;
                       dow[1][4].u.ms.high2  = size + array - 1;


                       // STEP 5
                       dow[0][5].type = BINSPLIT; 
                       dow[0][5].u.bs.regA = tmp;
                       dow[0][5].u.bs.regB = tmp + 2*quarter;
                       dow[0][5].u.bs.size = size;
                       dow[0][5].u.bs.target = array;
                       dow[0][5].u.bs.out_low_high2  =  &dow[0][6].u.ms.high2;
                       dow[0][5].u.bs.out_up_low2    =  &dow[1][6].u.ms.low2;
                       dow[0][5].u.bs.out_up_lowdest =  &dow[1][6].u.ms.lowdest;

                       dow[1][5].type = IDLE;

                       // STEP 6
                       dow[0][6].type = SEQMERGE;
                       dow[0][6].u.ms.low1   = tmp;
                       dow[0][6].u.ms.high1  = tmp + quarter - 1;
                       dow[0][6].u.ms.low2   = tmp + 2*quarter;
                       dow[0][6].u.ms.lowdest= array;

                       dow[1][6].type = SEQMERGE;
                       dow[1][6].u.ms.low1   = tmp + quarter + 1;
                       dow[1][6].u.ms.high1  = tmp + 2*quarter - 1;
                       dow[1][6].u.ms.high2  = tmp + size -1;

                }

                int cleanup(){ 
                    }

                int execute(int threadid)
                {
                int tid = threadid - leader;

                for(int s = 0; s < steps; s++){
                   quickmerge_uow *unit = &dow[tid][s];
                   switch (unit->type){
                      case SEQQUICK: quicksort(threadid, &unit->u.sq); break;
                      case SEQMERGE: mergesort(threadid, &unit->u.ms); break;
                      case BINSPLIT:   binpart(threadid, &unit->u.bs);   break;
                      case IDLE: break; // do nothing
                      }
                   barrier->wait();
                   }
                }
            
                int steps;
                quickmerge_uow dow[2][7]; // [threadid][step]
};

// like TAOQuickMerge, but without the quicksorts on the top
class TAOMerge2 : public AssemblyTask 
{
        public: 
                // initialize static parameters
                TAOMerge2(ELM *array, ELM *tmp, int size, int unused=0) : AssemblyTask(2) 
                {   
                       int quarter = size / 4;
                       steps = 5;

                       // STEP 0
                       dow[0][0].type = BINSPLIT; 
                       dow[0][0].u.bs.regA = array;
                       dow[0][0].u.bs.regB = array + quarter;
                       dow[0][0].u.bs.size = quarter*2;
                       dow[0][0].u.bs.target = tmp;
                       dow[0][0].u.bs.out_low_high2  =  &dow[0][1].u.ms.high2;
                       dow[0][0].u.bs.out_up_low2    =  &dow[1][1].u.ms.low2;
                       dow[0][0].u.bs.out_up_lowdest =  &dow[1][1].u.ms.lowdest;

                       dow[1][0].type = BINSPLIT; 
                       dow[1][0].u.bs.regA    = array + 2*quarter;
                       dow[1][0].u.bs.regB    = array + 3*quarter;
                       dow[1][0].u.bs.size    = size - quarter*2;
                       dow[1][0].u.bs.target  = tmp + 2*quarter;
                       dow[1][0].u.bs.out_low_high2  =  &dow[0][2].u.ms.high2;
                       dow[1][0].u.bs.out_up_low2    =  &dow[1][2].u.ms.low2;
                       dow[1][0].u.bs.out_up_lowdest =  &dow[1][2].u.ms.lowdest;

                       // STEP 1
                       dow[0][1].type = SEQMERGE;
                       dow[0][1].u.ms.low1   = array;
                       dow[0][1].u.ms.high1  = array + quarter/2 - 1;
                       dow[0][1].u.ms.low2   = array + quarter;
                       dow[0][1].u.ms.lowdest= tmp;

                       dow[1][1].type = SEQMERGE;
                       dow[1][1].u.ms.low1   = array + quarter/2 + 1;
                       dow[1][1].u.ms.high1  = array + quarter - 1;
                       dow[1][1].u.ms.high2  = array + quarter*2 -1;


                       // STEP 2
                       dow[0][2].type = SEQMERGE;
                       dow[0][2].u.ms.low1   = quarter*2 + array;
                       dow[0][2].u.ms.high1  = quarter*2 + array + quarter/2 - 1;
                       dow[0][2].u.ms.low2   = quarter*2 + array + quarter;
                       dow[0][2].u.ms.lowdest= quarter*2 + tmp;

                       dow[1][2].type = SEQMERGE;
                       dow[1][2].u.ms.low1   = quarter*2 + array + quarter / 2 + 1;
                       dow[1][2].u.ms.high1  = quarter*2 + array + quarter - 1;
                       dow[1][2].u.ms.high2  = size + array - 1;


                       // STEP 3
                       dow[0][3].type = BINSPLIT; 
                       dow[0][3].u.bs.regA = tmp;
                       dow[0][3].u.bs.regB = tmp + 2*quarter;
                       dow[0][3].u.bs.size = size;
                       dow[0][3].u.bs.target = array;
                       dow[0][3].u.bs.out_low_high2  =  &dow[0][4].u.ms.high2;
                       dow[0][3].u.bs.out_up_low2    =  &dow[1][4].u.ms.low2;
                       dow[0][3].u.bs.out_up_lowdest =  &dow[1][4].u.ms.lowdest;

                       dow[1][3].type = IDLE;

                       // STEP 4
                       dow[0][4].type = SEQMERGE;
                       dow[0][4].u.ms.low1   = tmp;
                       dow[0][4].u.ms.high1  = tmp + quarter - 1;
                       dow[0][4].u.ms.low2   = tmp + 2*quarter;
                       dow[0][4].u.ms.lowdest= array;

                       dow[1][4].type = SEQMERGE;
                       dow[1][4].u.ms.low1   = tmp + quarter + 1;
                       dow[1][4].u.ms.high1  = tmp + 2*quarter - 1;
                       dow[1][4].u.ms.high2  = tmp + size -1;
                }

                int cleanup(){ 
                    }

                int execute(int threadid)
                {
                int tid = threadid - leader;

                for(int s = 0; s < steps; s++){
                   quickmerge_uow *unit = &dow[tid][s];
                   switch (unit->type){
                      case SEQMERGE: mergesort(threadid, &unit->u.ms); break;
                      case BINSPLIT:   binpart(threadid, &unit->u.bs);   break;
                      case IDLE: break; // do nothing
                      }
                   barrier->wait();
                   }
                }
            
                int steps;
                quickmerge_uow dow[2][5]; // [threadid][step]
};

struct quickmerge_uow_dyn{
        int type; // SEQMERGE, SEQQUICK, BINSPLIT, IDLE
        union{
            struct seqmerge_uow ms;
            struct seqquick_uow sq;
            struct binsplit_uow bs;
            struct idle_uow     id;
        }u;
        std::list <quickmerge_uow_dyn *> out;
        std::atomic<int> refcount;
};
        
// like TAOMerge but using a dependence based scheduler with variable width
class TAOMergeDyn : public AssemblyTask 
{
        public: 
                // initialize static parameters
                TAOMergeDyn(ELM *array, ELM *tmp, int size, int res=4) : AssemblyTask(res) 
                {   
                       int quarter = size / 4;
                       // I don't think a barrier is needed
                       //barrier = new BARRIER(res);

                       pending_tasks = 9;

                       // entry 0
                       dow[0].type = BINSPLIT; 
                       dow[0].u.bs.regA = array;
                       dow[0].u.bs.regB = array + quarter;
                       dow[0].u.bs.size = quarter*2;
                       dow[0].u.bs.target = tmp;
                       dow[0].u.bs.out_low_high2  =   &dow[2].u.ms.high2;
                       dow[0].u.bs.out_up_low2    =   &dow[3].u.ms.low2;
                       dow[0].u.bs.out_up_lowdest =   &dow[3].u.ms.lowdest;
                       dow[0].out.push_back(&dow[2]); dow[2].refcount++;
                       dow[0].out.push_back(&dow[3]); dow[3].refcount++;


                       // entry 1
                       dow[1].type = BINSPLIT; 
                       dow[1].u.bs.regA    = array + 2*quarter;
                       dow[1].u.bs.regB    = array + 3*quarter;
                       dow[1].u.bs.size    = size - quarter*2;
                       dow[1].u.bs.target  = tmp + 2*quarter;
                       dow[1].u.bs.out_low_high2  =  &dow[4].u.ms.high2;
                       dow[1].u.bs.out_up_low2    =  &dow[5].u.ms.low2;
                       dow[1].u.bs.out_up_lowdest =  &dow[5].u.ms.lowdest;
                       dow[1].out.push_back(&dow[4]); dow[4].refcount++;
                       dow[1].out.push_back(&dow[5]); dow[5].refcount++;

                       // entry 2
                       dow[2].type = SEQMERGE;
                       dow[2].u.ms.low1   = array;
                       dow[2].u.ms.high1  = array + quarter/2 - 1;
                       dow[2].u.ms.low2   = array + quarter;
                       dow[2].u.ms.lowdest= tmp;
                       dow[2].out.push_back(&dow[6]); dow[6].refcount++;

                       // entry 3
                       dow[3].type = SEQMERGE;
                       dow[3].u.ms.low1   = array + quarter/2 + 1;
                       dow[3].u.ms.high1  = array + quarter - 1;
                       dow[3].u.ms.high2  = array + quarter*2 -1;
                       dow[3].out.push_back(&dow[6]); dow[6].refcount++;

                       // entry 4
                       dow[4].type = SEQMERGE;
                       dow[4].u.ms.low1   = quarter*2 + array;
                       dow[4].u.ms.high1  = quarter*2 + array + quarter/2 - 1;
                       dow[4].u.ms.low2   = quarter*2 + array + quarter;
                       dow[4].u.ms.lowdest= quarter*2 + tmp;
                       dow[4].out.push_back(&dow[6]); dow[6].refcount++;
 
                       // entry 5
                       dow[5].type = SEQMERGE;
                       dow[5].u.ms.low1   = quarter*2 + array + quarter / 2 + 1;
                       dow[5].u.ms.high1  = quarter*2 + array + quarter - 1;
                       dow[5].u.ms.high2  = size + array - 1;
                       dow[5].out.push_back(&dow[6]); dow[6].refcount++;

                       // entry 6
                       dow[6].type = BINSPLIT; 
                       dow[6].u.bs.regA = tmp;
                       dow[6].u.bs.regB = tmp + 2*quarter;
                       dow[6].u.bs.size = size;
                       dow[6].u.bs.target = array;
                       dow[6].u.bs.out_low_high2  =  &dow[7].u.ms.high2;
                       dow[6].u.bs.out_up_low2    =  &dow[8].u.ms.low2;
                       dow[6].u.bs.out_up_lowdest =  &dow[8].u.ms.lowdest;
                       dow[6].out.push_back(&dow[7]); dow[7].refcount++;
                       dow[6].out.push_back(&dow[8]); dow[8].refcount++;

                       // entry 7
                       dow[7].type = SEQMERGE;
                       dow[7].u.ms.low1   = tmp;
                       dow[7].u.ms.high1  = tmp + quarter - 1;
                       dow[7].u.ms.low2   = tmp + 2*quarter;
                       dow[7].u.ms.lowdest= array;

                       // entry 8
                       dow[8].type = SEQMERGE;
                       dow[8].u.ms.low1   = tmp + quarter + 1;
                       dow[8].u.ms.high1  = tmp + 2*quarter - 1;
                       dow[8].u.ms.high2  = tmp + size -1;

                       readyq.push_front(&dow[0]);
                       readyq.push_front(&dow[1]);
                }

                int cleanup(){ 
                    }

                int execute(int phys_id)
                {
                
                quickmerge_uow_dyn *unit = nullptr, *fwd = nullptr;

                int virt_id = phys_id - leader;
                while(pending_tasks > 0){
                    if(fwd) 
                            unit = fwd;
                    else{ 
                        LOCK_ACQUIRE(assembly_lock);
                        if(readyq.empty()){ // no ready tasks in the queue right now
                             LOCK_RELEASE(assembly_lock);
#ifdef PAUSE
                             _mm_pause();
#endif                        
                             continue;
                        }
                        unit = readyq.front();
                        readyq.pop_front();
                        LOCK_RELEASE(assembly_lock);
                    }
                    fwd = nullptr;

                    switch (unit->type){
                      case SEQMERGE: mergesort(phys_id, &unit->u.ms); break;
                      case BINSPLIT:   binpart(phys_id, &unit->u.bs); break;
                      }

                    pending_tasks.fetch_sub(1);

                    {
                    LOCK_ACQUIRE(assembly_lock);
                    while(!unit->out.empty()){
                            quickmerge_uow_dyn *next = unit->out.front();
                            int refs = next->refcount.fetch_sub(1);
                            if(refs == 1) {
                                    if(!fwd) fwd = next;
                                    else readyq.push_front(next);
                            }
                            unit->out.pop_front();
                    }
                    LOCK_RELEASE(assembly_lock);
                    }
                   }

                }
            
                std::atomic<int> pending_tasks;
                std::list <quickmerge_uow_dyn *> readyq;

                quickmerge_uow_dyn dow[9]; // 9 units of work

                // assembly locks
                GENERIC_LOCK(assembly_lock);
};


class TAOQuickMergeDyn : public AssemblyTask 
{
        public: 
#ifdef  TIME_TRACE             
                static double time_table[][3];
#endif
#ifdef  INT_SOL             
                static uint64_t time_table[][3];
#endif

                // initialize static parameters
                TAOQuickMergeDyn(ELM *array, ELM *tmp, int size, int res=2) : AssemblyTask(res) 
                {   
                       int quarter = size / 4;
                       pending_tasks = 13;

                       // STEP 0
                       // entry 0
                       dow[0].type = SEQQUICK;
                       dow[0].u.sq.low  = array + 0*quarter;
                       dow[0].u.sq.high = array + 1*quarter - 1;
                       dow[0].out.push_back(&dow[4]); dow[4].refcount++;

                       // entry 1
                       dow[1].type = SEQQUICK;
                       dow[1].u.sq.low  = array + 1*quarter;
                       dow[1].u.sq.high = array + 2*quarter - 1;
                       dow[1].out.push_back(&dow[4]); dow[4].refcount++;

                       // entry 2
                       dow[2].type = SEQQUICK;
                       dow[2].u.sq.low  = array + 2*quarter;
                       dow[2].u.sq.high = array + 3*quarter - 1;
                       dow[2].out.push_back(&dow[5]); dow[5].refcount++;

                       // entry 3
                       dow[3].type = SEQQUICK;
                       dow[3].u.sq.low  = array + 3*quarter;
                       dow[3].u.sq.high = array + size - 1;
                       dow[3].out.push_back(&dow[5]); dow[5].refcount++;

                       // STEP 4
                       dow[4].type = BINSPLIT; 
                       dow[4].u.bs.regA = array;
                       dow[4].u.bs.regB = array + quarter;
                       dow[4].u.bs.size = quarter*2;
                       dow[4].u.bs.target = tmp;
                       dow[4].u.bs.out_low_high2  =  &dow[6].u.ms.high2;
                       dow[4].u.bs.out_up_low2    =  &dow[7].u.ms.low2;
                       dow[4].u.bs.out_up_lowdest =  &dow[7].u.ms.lowdest;
                       dow[4].out.push_back(&dow[6]); dow[6].refcount++;
                       dow[4].out.push_back(&dow[7]); dow[7].refcount++;

                       // entry 5
                       dow[5].type = BINSPLIT; 
                       dow[5].u.bs.regA    = array + 2*quarter;
                       dow[5].u.bs.regB    = array + 3*quarter;
                       dow[5].u.bs.size    = size - quarter*2;
                       dow[5].u.bs.target  = tmp + 2*quarter;
                       dow[5].u.bs.out_low_high2  =  &dow[8].u.ms.high2;
                       dow[5].u.bs.out_up_low2    =  &dow[9].u.ms.low2;
                       dow[5].u.bs.out_up_lowdest =  &dow[9].u.ms.lowdest;
                       dow[5].out.push_back(&dow[8]); dow[8].refcount++;
                       dow[5].out.push_back(&dow[9]); dow[9].refcount++;

                       // entry 6
                       dow[6].type = SEQMERGE;
                       dow[6].u.ms.low1   = array;
                       dow[6].u.ms.high1  = array + quarter/2 - 1;
                       dow[6].u.ms.low2   = array + quarter;
                       dow[6].u.ms.lowdest= tmp;
                       dow[6].out.push_back(&dow[10]); dow[10].refcount++;

                       // entry 7
                       dow[7].type = SEQMERGE;
                       dow[7].u.ms.low1   = array + quarter/2 + 1;
                       dow[7].u.ms.high1  = array + quarter - 1;
                       dow[7].u.ms.high2  = array + quarter*2 -1;
                       dow[7].out.push_back(&dow[10]); dow[10].refcount++;

                       // STEP 8
                       dow[8].type = SEQMERGE;
                       dow[8].u.ms.low1   = quarter*2 + array;
                       dow[8].u.ms.high1  = quarter*2 + array + quarter/2 - 1;
                       dow[8].u.ms.low2   = quarter*2 + array + quarter;
                       dow[8].u.ms.lowdest= quarter*2 + tmp;
                       dow[8].out.push_back(&dow[10]); dow[10].refcount++;

                       // entry 9
                       dow[9].type = SEQMERGE;
                       dow[9].u.ms.low1   = quarter*2 + array + quarter / 2 + 1;
                       dow[9].u.ms.high1  = quarter*2 + array + quarter - 1;
                       dow[9].u.ms.high2  = size + array - 1;
                       dow[9].out.push_back(&dow[10]); dow[10].refcount++;

                       // entry 10
                       dow[10].type = BINSPLIT; 
                       dow[10].u.bs.regA = tmp;
                       dow[10].u.bs.regB = tmp + 2*quarter;
                       dow[10].u.bs.size = size;
                       dow[10].u.bs.target = array;
                       dow[10].u.bs.out_low_high2  =  &dow[11].u.ms.high2;
                       dow[10].u.bs.out_up_low2    =  &dow[12].u.ms.low2;
                       dow[10].u.bs.out_up_lowdest =  &dow[12].u.ms.lowdest;
                       dow[10].out.push_back(&dow[11]); dow[11].refcount++;
                       dow[10].out.push_back(&dow[12]); dow[12].refcount++;

                       // entry 11
                       dow[11].type = SEQMERGE;
                       dow[11].u.ms.low1   = tmp;
                       dow[11].u.ms.high1  = tmp + quarter - 1;
                       dow[11].u.ms.low2   = tmp + 2*quarter;
                       dow[11].u.ms.lowdest= array;

                       // entry 12
                       dow[12].type = SEQMERGE;
                       dow[12].u.ms.low1   = tmp + quarter + 1;
                       dow[12].u.ms.high1  = tmp + 2*quarter - 1;
                       dow[12].u.ms.high2  = tmp + size -1;

                       readyq.push_front(&dow[0]);
                       readyq.push_front(&dow[1]);
                       readyq.push_front(&dow[2]);
                       readyq.push_front(&dow[3]);
                }

                int cleanup(){ 
                    }

                int execute(int phys_id)
                {
                quickmerge_uow_dyn *unit = nullptr, *fwd = nullptr;

                int virt_id = phys_id - leader;
                while(pending_tasks > 0){
                    if(fwd) 
                            unit = fwd;
                    else{ 
                        LOCK_ACQUIRE(assembly_lock);
                        if(readyq.empty()){ // no ready tasks in the queue right now
                             LOCK_RELEASE(assembly_lock);
#ifdef PAUSE
                             _mm_pause();
#endif                        
                             continue;
                        }
                        unit = readyq.front();
                        readyq.pop_front();
                        LOCK_RELEASE(assembly_lock);
                    }
                    fwd = nullptr;

                    switch (unit->type){
                      case SEQQUICK: quicksort(phys_id, &unit->u.sq); break;
                      case SEQMERGE: mergesort(phys_id, &unit->u.ms); break;
                      case BINSPLIT:   binpart(phys_id, &unit->u.bs); break;
                      }

                    pending_tasks.fetch_sub(1);

                    {
                    LOCK_ACQUIRE(assembly_lock);
                    while(!unit->out.empty()){
                            quickmerge_uow_dyn *next = unit->out.front();
                            int refs = next->refcount.fetch_sub(1);
                            if(refs == 1) {
                                    if(!fwd) fwd = next;
                                    else readyq.push_front(next);
                            }
                            unit->out.pop_front();
                    }
                    LOCK_RELEASE(assembly_lock);
                    }
                   }

                }
#ifdef  INT_SOL             
              GENERIC_LOCK(ttable_lock);

              int set_timetable(int threadid, uint64_t ticks, int index){
//                  int index = (width == 4) ? (2) : ((width)-1);
                  LOCK_ACQUIRE(ttable_lock);
                  //  time_table[_res][threadid] = (d[_res][threadid] + ticks)/2;
                  time_table[threadid][index] = ticks;
                  LOCK_RELEASE(ttable_lock);
              }
#endif          
#ifdef  TIME_TRACE             
              GENERIC_LOCK(ttable_lock);

              int set_timetable(int threadid, double ticks, int index){
//                  int index = (width == 4) ? (2) : ((width)-1);
                  LOCK_ACQUIRE(ttable_lock);
                  //  time_table[_res][threadid] = (d[_res][threadid] + ticks)/2;
                  time_table[threadid][index] = ticks;
                  LOCK_RELEASE(ttable_lock);
              }
	     double get_timetable(int threadid, int index){
		 
                  double time = 0;
                  LOCK_ACQUIRE(ttable_lock);
		  time = time_table[threadid][index];
		  LOCK_RELEASE(ttable_lock);
	          return time;
	     }
#endif          
                std::atomic<int> pending_tasks;
                std::list <quickmerge_uow_dyn *> readyq;

                quickmerge_uow_dyn dow[13];   // 13 elements of work

                // assembly locks
                GENERIC_LOCK(assembly_lock);
};

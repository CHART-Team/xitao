// taosort.h

#include "tao.h"
#include <chrono>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <atomic>

#define SORT_BUFFER_SIZE (32*1024*1024)
#define MERGE_TASK_SIZE (2*1024)
#define SORT_TASK_SIZE (2*1024)
#define INSERTION_THR (20)
#define BARRIER spin_barrier

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

class TAOQuickMergeDyn : public AssemblyTask 
{
public: 
#if defined(CRIT_PERF_SCHED)
  static float time_table[][XITAO_MAXTHREADS];
#endif

                // initialize static parameters
  TAOQuickMergeDyn(ELM *array, ELM *tmp, int size, int res=2) : AssemblyTask(res) 
  {   
    int quarter = size / 4;
    pending_tasks = 13;

    assembly_lock.lock = false;

    for(int i = 0; i < 13; i++) dow[i].refcount=0;

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

 int execute(int phys_id){
  quickmerge_uow_dyn *unit = nullptr, *fwd = nullptr;

  int virt_id = phys_id - leader;
  while(pending_tasks > 0){
   if(fwd) 
    unit = fwd;
  else{ 
    LOCK_ACQUIRE(assembly_lock);
        if(readyq.empty()){ // no ready tasks in the queue right now
         LOCK_RELEASE(assembly_lock);                    
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
#if defined(CRIT_PERF_SCHED)
int set_timetable(int threadid, float ticks, int index){
  time_table[index][threadid] = ticks;
}
float get_timetable(int threadid, int index){
  float time = 0;
  time = time_table[index][threadid];
  return time;
}
#endif

std::atomic<int> pending_tasks;
std::list <quickmerge_uow_dyn *> readyq;
  quickmerge_uow_dyn dow[13];   // 13 elements of work
  // assembly locks
  GENERIC_LOCK(assembly_lock);
};

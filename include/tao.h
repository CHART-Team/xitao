// tao.h - Task Assembly Operator

#ifndef _TAO_H
#define _TAO_H

#include <sched.h>
#include <unistd.h>
#include <thread>
#include <iostream>
#include "lfq-fifo.h"
#include "config.h"
#include "xitao_api.h"
#include "poly_task.h"
#include "barriers.h"

extern int gotao_thread_base;
extern int gotao_nthreads;
extern int gotao_sys_topo[5];
extern int physcore[MAXTHREADS];

#define GET_TOPO_WIDTH_FROM_LEVEL(x) gotao_sys_topo[x]

typedef void (*task)(void *, int);
#define TASK_SIMPLE   0x0
#define TASK_ASSEMBLY 0x1

extern long int tao_total_steals;

//#define BARRIER cxx_barrier
#define BARRIER spin_barrier

// the base class for assemblies is very simple. It just provides base functionality for derived
// classes. The sleeping barrier is used by TAO to synchronize the start of assemblies
class AssemblyTask: public PolyTask{
public:
  AssemblyTask(int w, int nthread=0) : PolyTask(TASK_ASSEMBLY, nthread), leader(-1) {
    width = w;
#ifdef NEED_BARRIER
    barrier = new BARRIER(w);
#endif 
  }
#ifdef NEED_BARRIER
  BARRIER *barrier;
#endif
  int leader;
  virtual int execute(int thread) = 0;
  ~AssemblyTask(){
#ifdef NEED_BARRIER
    delete barrier;
#endif
  }  
};

class SimpleTask: public PolyTask{
public:
  SimpleTask(task fn, void *a, int nthread=0) : PolyTask(TASK_SIMPLE, nthread), args(a), f(fn){ 
    width = 1; 
  }
  void *args;
  task f;
};
#endif // _TAO_H

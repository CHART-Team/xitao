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
#include <atomic>
#include <functional>
// extern int gotao_thread_base;
// extern int gotao_sys_topo[5];
// extern int physcore[XITAO_MAXTHREADS];

#define GET_TOPO_WIDTH_FROM_LEVEL(x) gotao_sys_topo[x]

typedef void (*task)(void *, int);
#define TASK_SIMPLE   0x0
#define TASK_ASSEMBLY 0x1

// the base class for assemblies is very simple. It just provides base functionality for derived
// classes. The sleeping barrier is used by TAO to synchronize the start of assemblies
class AssemblyTask: public PolyTask{
public:
  AssemblyTask(int w, int nthread=0) : PolyTask(TASK_ASSEMBLY, nthread) {
    leader = -1;
    width = w;
#ifdef NEED_BARRIER
    barrier = new BARRIER(w);
#endif 
  }
#ifdef NEED_BARRIER
  BARRIER *barrier;
#endif  
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


template <typename IterType>
class ParForTask: public AssemblyTask {
private:
#if defined(CRIT_PERF_SCHED)
  static float time_table[XITAO_MAXTHREADS][XITAO_MAXTHREADS];
#endif  
  std::function<void(int&, int&)> _spmd_region;
  IterType _start;
  IterType _end;
  IterType _block_size; 
  IterType _block_iter;
  IterType _blocks;
  std::atomic<int> next; /*!< TAO implementation specific atomic variable to provide thread safe tracker of the number of processed blocks */
  const size_t slackness = 4;
public:
  ParForTask(int width, IterType start, IterType end, std::function<void(int&, int&)> spmd): AssemblyTask(width), _start(start), _end(end), _spmd_region(spmd) { 
    IterType size = end - start; 
    _block_iter = 0;
    _block_size = size / (width * slackness);
    if(_block_size <= 0) _block_size = 1;
    _blocks = size / _block_size;
    next = 0;
  }

  int execute(int thread) {
    int block_id = next++;
    while(block_id < _blocks) {
      for(int i = block_id * _block_size; (i < _end) && (i < (block_id + 1) * _block_size); i++)
        _spmd_region(i, thread);
      block_id = next++;
    }
  } 
  int cleanup() { }
#if defined(CRIT_PERF_SCHED)
  //! Inherited pure virtual function that is called by the performance based scheduler to set an entry in PTT
  /*!
    \param threadid logical thread id that executes the TAO
    \param ticks the number of elapsed ticks
    \param index the index of the width type
    \sa time_table()
  */
  int set_timetable(int threadid, float ticks, int index) {
    time_table[index][threadid] = ticks;
  }
  
  //! Inherited pure virtual function that is called by the performance based scheduler to get an entry in PTT
  /*!
    \param threadid logical thread id that executes the TAO
    \param index the index of the width type
    \sa time_table()
  */
  float get_timetable(int threadid, int index) { 
    float time=0;
    time = time_table[index][threadid];
    return time;
  }
#endif
};
#if defined(CRIT_PERF_SCHED)
template <typename IterType>
float ParForTask<IterType>::time_table[XITAO_MAXTHREADS][XITAO_MAXTHREADS]; 
#endif
#endif // _TAO_H

// tao.h - Task Assembly Operator

#ifndef _TAO_H
#define _TAO_H

#include <sched.h>
#include <unistd.h>
#include <thread>
#include <iostream>
#include "config.h"
//#include "xitao_api.h"
#include "xitao_workspace.h"
#include "poly_task.h"
#include "barriers.h"
#include <atomic>
#include <chrono>
#include <functional>
using namespace xitao;
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
  virtual void execute(int thread) = 0;
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
template<class F>
struct xitao_looper {
  F fn;
  void operator()(int start, int end, int thread) const {
    for (int i = start; i < end; ++i)
     fn(i, thread);    
  }
};

template<class F>
xitao_looper<F> looper(F f) {
  return {std::move(f)};
}


// a "ParForTask" that executes a partition on an SPMD region with either dynamic or static scheduling
template <typename FuncType, typename IterType>
class ParForTask: public AssemblyTask {  
private:
  int const _sched_type;
  IterType _start;
  IterType _end;
  FuncType  _spmd_region;  
  IterType _block_size; 
  IterType _block_iter;
  IterType _blocks; 
  IterType _size; 
  std::atomic<int> next_block; /*!< TAO implementation specific atomic variable to provide thread safe tracker of the number of processed blocks */
  const size_t slackness = 8;
public: 
  ParForTask(int sched, IterType start, IterType end, FuncType spmd, int width): 
                AssemblyTask(width), _sched_type(sched), _start(start), _end(end), _spmd_region(spmd) { 
    _size = _end - _start; 
    if(_size <= 0) {
      std::cout << "Error: no work to be done" << std::endl;
      exit(0);
    }
    _block_iter = 0;
    next_block = 0;
    switch(_sched_type) {
      case xitao_vec_static: break;
      case xitao_vec_dynamic: 
        _block_size = _size / (width * slackness);
        if(_block_size <= 0) _block_size = 1;
        _blocks = (_size + _block_size -1) / _block_size;
      break;
      default:
        std::cout << "Error: undefined sched type in XiTAO vector code" << std::endl;
        exit(0);
    }
  }

  void execute(int thread) {   
    if(_sched_type == xitao_vec_dynamic) {
      int block_id = next_block++;
      while(block_id < _blocks) {
        int local_block_start = _start + block_id * _block_size;
        int local_block_end   = (block_id >= _blocks - 1)? _end : local_block_start + _block_size;
        _spmd_region(local_block_start, local_block_end, thread);
        block_id = next_block++;
      }
    } else { // xitao_vec_static
      _block_size = _size / width;
      if(_block_size < 1) {
        std::cout << "Error: not enough work to do" << std::endl;
        exit(0);
      }      
      int thread_id = thread - leader;
      int local_block_start = _start + thread_id * _block_size;
      int local_block_end   = (thread_id >= width - 1)? _end : local_block_start + _block_size;
      _spmd_region(local_block_start, local_block_end, thread);
    }
  } 
  void cleanup() { }
};
#endif // _TAO_H

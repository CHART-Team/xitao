#ifndef XITAO_WORKSPACE
#define XITAO_WORKSPACE

#include <list>
#include <vector>
#include <iomanip>
#include <atomic>
#include <thread>
#include "lfq-fifo.h"
#include "barriers.h"
#include "config.h"
//! A cache-aligned boolean type to implement locks
struct aligned_lock {
  std::atomic<bool> lock __attribute__((aligned(64)));
};

//! A cache-aligned integer to track the number of completed tasks by a master thread
struct completions{
  int tasks __attribute__((aligned(64)));
};
//#define BARRIER cxx_barrier
#define BARRIER spin_barrier
#define GENERIC_LOCK(l)  aligned_lock l;
#define LOCK_ACQUIRE(l)  while(l.lock.exchange(true)) {while(l.lock.load()){ }}
#define LOCK_RELEASE(l)  l.lock.store(false);
const int xitao_vec_static = 0;
const int xitao_vec_dynamic = 1;
class PolyTask;
namespace xitao {     
  extern std::list<PolyTask *> worker_ready_q[XITAO_MAXTHREADS];
  extern LFQueue<PolyTask *> worker_assembly_q[XITAO_MAXTHREADS];  
  extern long int tao_total_steals;  
  extern BARRIER *starting_barrier;
  extern cxx_barrier *tao_barrier;
  extern std::vector<int> static_resource_mapper;
  extern std::vector<std::vector<int> > ptt_layout;
  extern std::vector<std::vector<std::pair<int, int> > > inclusive_partitions;
  extern struct completions task_completions[XITAO_MAXTHREADS];
  extern struct completions task_pool[XITAO_MAXTHREADS];
  extern int critical_path;
  extern int gotao_nthreads;
  extern int gotao_ncontexts;
  extern int gotao_thread_base;
  extern bool gotao_can_exit;
  extern bool gotao_initialized;
  extern bool gotao_started;
  extern bool resources_runtime_conrolled;
  extern bool suppress_init_warnings;
  extern std::vector<int> runtime_resource_mapper;                                   // a logical to physical runtime resource mapper
  extern std::thread *t[XITAO_MAXTHREADS];
  extern std::mutex smpd_region_lock;  
  extern GENERIC_LOCK(worker_lock[XITAO_MAXTHREADS]);
  extern GENERIC_LOCK(worker_assembly_lock[XITAO_MAXTHREADS]);
#ifdef DEBUG
  extern GENERIC_LOCK(output_lck);
#endif
};
#endif
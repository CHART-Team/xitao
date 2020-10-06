#ifndef XITAO_WORKSPACE
#define XITAO_WORKSPACE

#include <list>
#include <vector>
#include <iomanip>
#include <atomic>
#include <thread>
#include <unordered_map>
#include "lfq-fifo.h"
#include "barriers.h"
#include "config.h"
#include "xitao_ptt_key.h"

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
//struct tao_type_info;
namespace xitao {   
  //typedef std::pair<std::type_index, size_t> ptt_key_type;
  typedef xitao_ptt_key ptt_key_type;

   /*! a typedef of underlying ptt table type*/
  typedef std::vector<float> ptt_value_type; 

  /*! a typedef of the shared pointer to ptt table type*/
  typedef std::shared_ptr<ptt_value_type> ptt_shared_type;  

  /*! a typedef of map that keeps track of the active PTT tables*/
  typedef std::unordered_map<ptt_key_type, ptt_shared_type, xitao_ptt_hash> tmap;    

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
  extern bool resources_runtime_controlled;
  extern bool suppress_init_warnings;
  extern std::vector<int> runtime_resource_mapper;                                   // a logical to physical runtime resource mapper
  extern std::thread *t[XITAO_MAXTHREADS];
  extern std::mutex smpd_region_lock;  
  extern  std::mutex pending_tasks_mutex;
  extern std::condition_variable pending_tasks_cv;
  extern GENERIC_LOCK(worker_lock[XITAO_MAXTHREADS]);
  extern GENERIC_LOCK(worker_assembly_lock[XITAO_MAXTHREADS]);
#ifdef DEBUG
  extern GENERIC_LOCK(output_lck);
#endif
};
#endif

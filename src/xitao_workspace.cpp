#include "xitao_workspace.h"
namespace xitao {
  // std::vector<PolyTask *> worker_ready_q[XITAO_MAXTHREADS];
  // LFQueue<PolyTask *> worker_assembly_q[XITAO_MAXTHREADS];
  long int tao_total_steals = 0;  
  BARRIER* starting_barrier;
  cxx_barrier* tao_barrier;  
  struct completions task_completions[XITAO_MAXTHREADS];
  struct completions task_pool[XITAO_MAXTHREADS];
  int critical_path;
  int xitao_nthreads;
  int gotao_ncontexts;
  int gotao_thread_base;
  bool gotao_can_exit = false;
  bool gotao_initialized = false;
  bool gotao_started = false;
  bool resources_runtime_controlled = false;
  bool suppress_init_warnings = false;
  std::vector<int> runtime_resource_mapper;                                   // a logical to physical runtime resource mapper
  std::thread* t[XITAO_MAXTHREADS];
  std::vector<int> static_resource_mapper(XITAO_MAXTHREADS);
  std::vector<std::vector<int> > ptt_layout(XITAO_MAXTHREADS);
  std::vector<std::vector<std::pair<int, int> > > inclusive_partitions(XITAO_MAXTHREADS);
  GENERIC_LOCK(worker_lock[XITAO_MAXTHREADS]);
  GENERIC_LOCK(worker_assembly_lock[XITAO_MAXTHREADS]);
  std::mutex smpd_region_lock;
  std::mutex pending_tasks_mutex;
  std::condition_variable pending_tasks_cv;
#ifdef DEBUG
  GENERIC_LOCK(output_lck);
#endif
}

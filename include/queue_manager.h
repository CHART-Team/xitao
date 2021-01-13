/*! \file 
@brief Handles all work-stealing and assembly queue insertions, and deletions.
*/

#ifndef QUEUE_MANAGER
#define QUEUE_MANAGER
#include "xitao_workspace.h"
#include "tao.h"
#include "debug_info.h"
#include "perf_model.h"
#include <queue>
namespace xitao {
  template <typename ready_queue_type, typename assembly_queue_type >
  class queue_manager {
    private: 
   
      static ready_queue_type worker_ready_q[XITAO_MAXTHREADS];  
      static assembly_queue_type worker_assembly_q[XITAO_MAXTHREADS]; 
      typedef typename ready_queue_type::value_type queue_element_type;

    public:
      static inline void insert_task_in_assembly_queues(queue_element_type task) {
        DEBUG_MSG(                                                                 \
          "[DEBUG] Distributing assembly task " << task->taskid << " with width "  \
           << task->width << " to workers [" << task->leader << "," << task->leader\
            + task->width << ")");
        for(int i = task->leader; i < task->leader + task->width; i++){
          assert(i < config::nthreads && i >= 0);
          LOCK_ACQUIRE(worker_assembly_lock[i]);
          worker_assembly_q[i].push(task);
        }
        for(int i = task->leader; i < task->leader + task->width; i++){
          LOCK_RELEASE(worker_assembly_lock[i]);
        }  
      }

      static inline void insert_in_ready_queue(queue_element_type task, int nthread) {
        LOCK_ACQUIRE(worker_lock[nthread]);
        worker_ready_q[nthread].push_back(task);
        LOCK_RELEASE(worker_lock[nthread]);
      }

      static inline bool try_pop_assembly_task(int nthread, queue_element_type &task) {
        if(!worker_assembly_q[nthread].empty()) {
          task = worker_assembly_q[nthread].front();
          worker_assembly_q[nthread].pop();
          return true;
        } else {
          return false;
        }
      }

      static inline bool try_pop_ready_task(int nthread, queue_element_type &task, int requesting_thread) {
        LOCK_ACQUIRE(worker_lock[nthread]);
        if(!worker_ready_q[nthread].empty()) {
          task = worker_ready_q[nthread].back(); 
          worker_ready_q[nthread].pop_back();
          LOCK_RELEASE(worker_lock[nthread]);
          if(config::use_performance_modeling)
            perf_model::history_mold_locally(requesting_thread, task);
          DEBUG_MSG("[DEBUG] Priority=0, task "<< task->taskid <<" will run on thread "<< task->leader << ", width become " << task->width);
          return true;
        } else {
          LOCK_RELEASE(worker_lock[nthread]);
          return false;
        } 
      } 
  };
  // To try another data structure (e.g. list, deque or LFQueue), change the declaration below
  // TODO: compile-time checking of existence of functions (e.g. push_back, pop_back...)
  using default_queue_manager = queue_manager<std::vector<PolyTask*>, LFQueue<PolyTask*>>;
  //using default_queue_manager = queue_manager<PolyTask* , std::list<PolyTask*>, LFQueue<PolyTask*>>;

  template <typename ready_queue_type, typename assembly_queue_type > 
  ready_queue_type queue_manager<ready_queue_type, assembly_queue_type>::worker_ready_q[XITAO_MAXTHREADS];

  template <typename ready_queue_type, typename assembly_queue_type >
  assembly_queue_type queue_manager<ready_queue_type, assembly_queue_type>::worker_assembly_q[XITAO_MAXTHREADS]; 
}



#endif

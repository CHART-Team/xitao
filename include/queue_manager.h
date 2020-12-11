#ifndef QUEUE_MANAGER
#define QUEUE_MANAGER
#include "xitao_workspace.h"
#include "tao.h"
#include "debug_info.h"
namespace xitao {
  class queue_manager {
    private: 
      static std::vector<PolyTask *> worker_ready_q[XITAO_MAXTHREADS];
      static LFQueue<PolyTask *> worker_assembly_q[XITAO_MAXTHREADS]; 
    
    public:
      static inline void insert_task_in_assembly_queues(PolyTask* task) {
        DEBUG_MSG(                                                                 \
          "[DEBUG] Distributing assembly task " << task->taskid << " with width "  \
           << task->width << " to workers [" << task->leader << "," << task->leader\
            + task->width << ")");
        for(int i = task->leader; i < task->leader + task->width; i++){
          LOCK_ACQUIRE(worker_assembly_lock[i]);
          worker_assembly_q[i].push_back(task);
        }
        for(int i = task->leader; i < task->leader + task->width; i++){
          LOCK_RELEASE(worker_assembly_lock[i]);
        }  
      }

      static inline void insert_in_ready_queue(PolyTask* task, int nthread) {
        LOCK_ACQUIRE(worker_lock[nthread]);
        worker_ready_q[nthread].push_back(task);
        LOCK_RELEASE(worker_lock[nthread]);
      }

      static inline bool try_pop_assembly_task(int nthread, PolyTask* &task) {
        return worker_assembly_q[nthread].pop_front(&task);
      }

      static inline bool try_pop_ready_task(int nthread, PolyTask* &task) {
        LOCK_ACQUIRE(worker_lock[nthread]);
        if(!worker_ready_q[nthread].empty()) {
          task = worker_ready_q[nthread].back(); 
          worker_ready_q[nthread].pop_back();
          LOCK_RELEASE(worker_lock[nthread]);
#if defined(CRIT_PERF_SCHED)
          task->history_mold(nthread, task);
#endif      
          DEBUG_MSG("[DEBUG] Priority=0, task "<< task->taskid <<" will run on thread "<< task->leader << ", width become " << task->width);
          return true;
        } else {
          LOCK_RELEASE(worker_lock[nthread]);
          return false;
        } 
      } 
  };
}




#endif
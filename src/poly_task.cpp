#include "poly_task.h"
#include "debug_info.h"
#include <stdlib.h>
#include <iostream>
#include <iomanip>
#include <algorithm>
#if defined(DEBUG)
#include <iostream>
#endif
#include "xitao_workspace.h"
#include "queue_manager.h"
// #include "perf_model.h"
using namespace xitao;

//The pending PolyTasks count 
std::atomic<int> PolyTask::pending_tasks;

// need to declare the static class memeber
#if defined(DEBUG)
std::atomic<int> PolyTask::created_tasks;
#endif

PolyTask::PolyTask(int t, int _nthread=0) : type(t){
  refcount = 0;
#define GOTAO_NO_AFFINITY (1.0)
  affinity_relative_index = GOTAO_NO_AFFINITY;
  affinity_queue = -1;
#if defined(DEBUG) 
  taskid = created_tasks += 1;
#endif
  LOCK_ACQUIRE(worker_lock[_nthread]);
  if(task_pool[_nthread].tasks == 0) {
    pending_tasks += TASK_POOL;
    task_pool[_nthread].tasks = TASK_POOL - 1;
    DEBUG_MSG("[DEBUG] Requested: " << TASK_POOL << " tasks. Pending is now: " << pending_tasks);
  }
  else task_pool[_nthread].tasks--;
  LOCK_RELEASE(worker_lock[_nthread]);
  threads_out_tao = 0;
#if defined(CRIT_PERF_SCHED)
  criticality=0;
  marker = 0;  
  // setting the hint to 0. can be overwritten by the constructor of the child class 
  workload_hint = 0;
#endif
}
                                          // Internally, GOTAO works only with queues, not stas
int PolyTask::sta_to_queue(float x){
  if(x >= GOTAO_NO_AFFINITY){ 
    affinity_queue = -1;
  }
    else if (x < 0.0) return 1;  // error, should it be reported?
    else affinity_queue = (int) (x*xitao_nthreads);
    return 0; 
  }
int PolyTask::set_sta(float x){    
  affinity_relative_index = x;  // whenever a sta is changed, it triggers a translation
  return sta_to_queue(x);
} 
float PolyTask::get_sta(){             // return sta value
  return affinity_relative_index; 
}    
int PolyTask::clone_sta(PolyTask *pt) { 
  affinity_relative_index = pt->affinity_relative_index;    
  affinity_queue = pt->affinity_queue; // make sure to copy the exact queue
  return 0;
}
void PolyTask::make_edge(PolyTask *t){
  out.push_back(t);
  t->refcount++;
#ifdef CRIT_PERF_SCHED    
  t->_ptt = xitao_ptt::try_insert_table(t, t->workload_hint);               /*be sure that the dependent task has a PTT*/  
#endif    
}

#if defined(CRIT_PERF_SCHED)
// void PolyTask::history_mold(int _nthread, PolyTask *it){
//   int new_width = 1; 
//   int new_leader = -1;
//   float shortest_exec = 1000.0f;
//   float comp_perf = 0.0f; 
//   auto&& partitions = inclusive_partitions[_nthread];
//   if(rand()%10 != 0) { 
//     for(auto&& elem : partitions) {
//       int leader = elem.first;
//       int width  = elem.second;
//       auto&& ptt_val = it->get_timetable(leader, width - 1);
//       if(ptt_val == 0.0f) {
//         new_width = width;
//         new_leader = leader;       
//         break;
//       }
//       comp_perf = width * ptt_val;
//       //comp_perf = ptt_val;
//       if (comp_perf < shortest_exec) {
//         shortest_exec = comp_perf;
//         new_width = width;
//         new_leader = leader;      
//       }
//     }
//   } else { 
//     auto&& rand_partition = partitions[rand() % partitions.size()];
//     new_leader = rand_partition.first;
//     new_width  = rand_partition.second;
//   }
//   if(new_leader != -1) {
//     it->leader = new_leader;
//     it->width  = new_width;
//   }
// } 

//Determine if task is critical task
int PolyTask::if_prio(int _nthread, PolyTask * it){
  return it->criticality;
} 

// get value at specific location (leader, width) in ptt
float PolyTask::get_timetable(int thread, int index) {
  // return the ptt measurement at location
  return (*_ptt)[index * XITAO_MAXTHREADS + thread];
}

// set value at specific location (leader, width) in ptt
void PolyTask::set_timetable(int thread, float ticks, int index) {
  (*_ptt)[index * XITAO_MAXTHREADS + thread] = ticks;  
}

// void PolyTask::globalsearch_PTT(int nthread, PolyTask * it){
//   float shortest_exec = 1000.0f;
//   float comp_perf = 0.0f; 
//   int new_width = 1; 
//   int new_leader = -1;
//   for(int leader = 0; leader < ptt_layout.size(); ++leader) {
//     for(auto&& width : ptt_layout[leader]) {
//       if(width <= 0) continue;
//       auto&& ptt_val = it->get_timetable(leader, width - 1);
//       if(ptt_val == 0.0f) {
//         new_width = width;
//         new_leader = leader; 
//         leader = ptt_layout.size(); 
//         break;
//       }
//       comp_perf = width * ptt_val;
//       if (comp_perf < shortest_exec) {
//         shortest_exec = comp_perf;
//         new_width = width;
//         new_leader = leader;      
//       }
//     }
//   }  
//   it->width = new_width; 
//   it->leader = new_leader;
// }
#endif

PolyTask * PolyTask::commit_and_wakeup(int _nthread){
  PolyTask *ret = nullptr;
  for(auto&& it : out) {
    int refs = it->refcount.fetch_sub(1);
    if(refs == 1){
      DEBUG_MSG("[DEBUG] Task " << it->taskid << " became ready");
#if defined(CRIT_PERF_SCHED)
      int pr = if_prio(_nthread, it);
      if (pr == 1){
        perf_model::global_search_ptt(it);
        //globalsearch_PTT(_nthread, it);
        DEBUG_MSG("[DEBUG] Priority=1, task "<< it->taskid <<" will run on thread "<< it->leader << ", width become " << it->width);
        default_queue_manager::insert_task_in_assembly_queues(it);
      }
      else {  
        DEBUG_MSG("[DEBUG] Priority=0, task "<< it->taskid <<" is pushed to WSQ of thread "<< _nthread); 
        default_queue_manager::insert_in_ready_queue(it, _nthread);
      }
                
#else
      if(!ret && ((it->affinity_queue == -1) || ((it->affinity_queue/it->width) == (_nthread/it->width)))){
        // history_mold(_nthread,(*it)); 
        ret = it; // forward locally only if affinity matches
      }
      else{
        // otherwise insert into affinity queue, or in local queue
        int ndx = it->affinity_queue;
        if((ndx == -1) || ((it->affinity_queue/it->width) == (_nthread/it->width)))
          ndx = _nthread;

        //history_mold(_nthread,(*it)); 
        
        default_queue_manager::insert_in_ready_queue(it, ndx);
      } 
#endif
    }
  }
  task_completions[_nthread].tasks++;
  return ret;
}       

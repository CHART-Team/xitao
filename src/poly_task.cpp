/* BSD 3-Clause License

* Copyright (c) 2019-2021, contributors
* All rights reserved.

* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:

* 1. Redistributions of source code must retain the above copyright notice, this
*    list of conditions and the following disclaimer.

* 2. Redistributions in binary form must reproduce the above copyright notice,
*    this list of conditions and the following disclaimer in the documentation
*    and/or other materials provided with the distribution.

* 3. Neither the name of the copyright holder nor the names of its
*    contributors may be used to endorse or promote products derived from
*    this software without specific prior written permission.

* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

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

PolyTask::PolyTask(int t, int _nthread=0) : type(t) {
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
  threads_in_tao = 0;

  criticality=0;
  marker = 0;  
  // setting the hint to 0. can be overwritten by the constructor of the child class 
  workload_hint = 0;
  mold = true;
}
                                          // Internally, GOTAO works only with queues, not stas
void PolyTask::sta_to_queue(float x) {
  if(x >= GOTAO_NO_AFFINITY || x < 0.0) { 
    affinity_queue = -1;
  } else {
    affinity_queue = (int) (x*xitao_nthreads);
  }
}

void PolyTask::set_sta(float x) {  
  if(config::sta == 1) {
    affinity_relative_index = x;  // whenever a sta is changed, it triggers a translation
    sta_to_queue(x);
  }
} 
float PolyTask::get_sta() {             // return sta value
  return affinity_relative_index; 
}    
int PolyTask::clone_sta(PolyTask *pt) { 
  affinity_relative_index = pt->affinity_relative_index;    
  affinity_queue = pt->affinity_queue; // make sure to copy the exact queue
  return 0;
}
void PolyTask::make_edge(PolyTask *t) {
  out.push_back(t);
  t->refcount++;
  if(config::use_performance_modeling)    
    t->_ptt = xitao_ptt::try_insert_table(t, t->workload_hint);               /*be sure that the dependent task has a PTT*/   
}

//Determine if task is critical task
int PolyTask::if_prio(int _nthread, PolyTask * it) {
  return it->criticality;
} 

// get value at specific location (leader, width) in ptt
float PolyTask::get_timetable(int thread, int index) {
  // return the ptt measurement at location
  return _ptt->data[index * xitao_ptt::ptt_row_size + thread];
}

// set value at specific location (leader, width) in ptt
void PolyTask::set_timetable(int thread, float ticks, int index) {
  _ptt->data[index * xitao_ptt::ptt_row_size + thread] = ticks;  
}

PolyTask * PolyTask::commit_and_wakeup(int _nthread) {
  PolyTask *ret = nullptr;
  for(auto&& it : out) {
    int refs = it->refcount.fetch_sub(1);
    if(refs == 1) {
      DEBUG_MSG("[DEBUG] Task " << it->taskid << " became ready");
      if(it->criticality == 1 && config::use_performance_modeling) {
        perf_model::global_search_ptt(it);
        DEBUG_MSG("[DEBUG] Priority=1, task "<< it->taskid <<" will run on thread "<< it->leader << ", width become " << it->width);
        default_queue_manager::insert_task_in_assembly_queues(it);
      } else {
        // if(!ret && it->affinity_queue == -1){
        //   ret = it;
        // } else { 
          // either forward locally or following affinity
          int ndx = (it->affinity_queue == -1)? _nthread : it->affinity_queue;
          default_queue_manager::insert_in_ready_queue(it, ndx);
        //} 
      }
    }
  }
  task_completions[_nthread].tasks++;
  return ret;
}       

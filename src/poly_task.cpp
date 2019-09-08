#include "poly_task.h"
#include <stdlib.h>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <vector>
#if defined(DEBUG)
#include <iostream>
#endif
//extern int TABLEWIDTH;
extern int gotao_nthreads;
extern std::vector<int> static_resource_mapper;
extern std::vector<std::vector<int> > ptt_layout;
extern std::vector<std::vector<std::pair<int, int> > > inclusive_partitions;
extern GENERIC_LOCK(worker_assembly_lock[XITAO_MAXTHREADS]);
extern LFQueue<PolyTask *> worker_assembly_q[XITAO_MAXTHREADS];
//extern int wid[XITAO_MAXTHREADS];
extern aligned_lock worker_lock[XITAO_MAXTHREADS];
extern int critical_path;
extern std::list<PolyTask *> worker_ready_q[XITAO_MAXTHREADS];
extern completions task_completions[XITAO_MAXTHREADS];
extern completions task_pool[XITAO_MAXTHREADS];
extern int TABLEWIDTH;
#ifdef DEBUG
extern GENERIC_LOCK(output_lck);
#endif

//The pending PolyTasks count 
std::atomic<int> PolyTask::pending_tasks;

#ifdef WEIGHT_SCHED
//The current threshold value for weight-based of the system
std::atomic<double> PolyTask::bias;
#endif
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
  if(task_pool[_nthread].tasks == 0){
    pending_tasks += TASK_POOL;
    task_pool[_nthread].tasks = TASK_POOL-1;
      #ifdef DEBUG
    std::cout << "[DEBUG] Requested: " << TASK_POOL << " tasks. Pending is now: " << pending_tasks << "\n";
      #endif
  }
  else task_pool[_nthread].tasks--;
  LOCK_RELEASE(worker_lock[_nthread]);
  threads_out_tao = 0;
#if defined(CRIT_PERF_SCHED)
  criticality=0;
  marker = 0;
#endif
}
                                          // Internally, GOTAO works only with queues, not stas
int PolyTask::sta_to_queue(float x){
  if(x >= GOTAO_NO_AFFINITY){ 
    affinity_queue = -1;
  }
    else if (x < 0.0) return 1;  // error, should it be reported?
    else affinity_queue = (int) (x*gotao_nthreads);
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
}

  //History-based molding
#if defined(CRIT_PERF_SCHED)
int PolyTask::history_mold(int _nthread, PolyTask *it){
  int new_width = 1; 
  int new_leader = -1;
  float shortest_exec = 1000.0f;
  float comp_perf = 0.0f; 
  auto&& partitions = inclusive_partitions[_nthread];
  if(rand()%10 != 0) { 
    for(auto&& elem : partitions) {
      int leader = elem.first;
      int width  = elem.second;
      auto&& ptt_val = it->get_timetable(leader, width - 1);
      if(ptt_val == 0.0f) {
        new_width = width;
        new_leader = leader;       
        break;
      }
      comp_perf = width * ptt_val;
      if (comp_perf < shortest_exec) {
        shortest_exec = comp_perf;
        new_width = width;
        new_leader = leader;      
      }
    }
  } else { 
    auto&& rand_partition = partitions[rand() % partitions.size()];
    new_leader = rand_partition.first;
    new_width  = rand_partition.second;
  }
  if(new_leader != -1) {
    it->leader = new_leader;
    it->width  = new_width;
  }
} 
  //Recursive function assigning criticality
int PolyTask::set_criticality(){
  if((criticality)==0){
    int max=0;
    for(std::list<PolyTask *>::iterator edges = out.begin();edges != out.end();++edges){
      int new_max =((*edges)->set_criticality());
      max = ((new_max>max) ? (new_max) : (max));
    }
    criticality=++max;
  } 
  return criticality;
}

int PolyTask::set_marker(int i){
  for(std::list<PolyTask *>::iterator edges = out.begin(); edges != out.end(); ++edges){
    if((*edges) -> criticality == critical_path - i){
      (*edges) -> marker = 1;
      i++;
      (*edges) -> set_marker(i);
      break;
    }
  }
  return marker;
}


//Determine if task is critical task
int PolyTask::if_prio(int _nthread, PolyTask * it){
  return it->criticality;
} 

void PolyTask::print_ptt(float table[][XITAO_MAXTHREADS], const char* table_name) { 
  std::cout << std::endl << table_name <<  " PTT Stats: " << std::endl;
  for(int leader = 0; leader < ptt_layout.size() && leader < gotao_nthreads; ++leader) {
    auto row = ptt_layout[leader];
    std::sort(row.begin(), row.end());
    std::ostream time_output (std::cout.rdbuf());
    std::ostream scalability_output (std::cout.rdbuf());
    std::ostream width_output (std::cout.rdbuf());
    std::ostream empty_output (std::cout.rdbuf());
    time_output  << std::scientific << std::setprecision(3);
    scalability_output << std::setprecision(3);    
    empty_output << std::left << std::setw(5);
    std::cout << "Thread = " << leader << std::endl;    
    std::cout << "==================================" << std::endl;
    std::cout << " | " << std::setw(5) << "Width" << " | " << std::setw(9) << std::left << "Time" << " | " << "Scalability" << std::endl;
    std::cout << "==================================" << std::endl;
    for (int i = 0; i < row.size(); ++i){
      int curr_width = row[i];
      auto comp_perf = table[curr_width - 1][leader];
      std::cout << " | ";
      width_output << std::left << std::setw(5) << curr_width;
      std::cout << " | ";      
      time_output << comp_perf; 
      std::cout << " | ";
      if(i == 0) {        
        empty_output << " - ";
      } else if(comp_perf != 0.0f) {
        auto scaling = table[row[0] - 1][leader]/comp_perf;
        auto efficiency = scaling / curr_width;
        if(efficiency  < 0.6 || efficiency > 1.3) {
          scalability_output << "(" <<table[row[0] - 1][leader]/comp_perf << ")";  
        } else {
          scalability_output << table[row[0] - 1][leader]/comp_perf;
        }
      }
      std::cout << std::endl;  
    }
    std::cout << std::endl;
  }
}  

int PolyTask::globalsearch_PTT(int nthread, PolyTask * it){
  float shortest_exec = 1000.0f;
  float comp_perf = 0.0f; 
  int new_width = 1; 
  int new_leader = -1;
  for(int leader = 0; leader < ptt_layout.size(); ++leader) {
    for(auto&& width : ptt_layout[leader]) {
      auto&& ptt_val = it->get_timetable(leader, width - 1);
      if(ptt_val == 0.0f) {
        new_width = width;
        new_leader = leader; 
        leader = ptt_layout.size(); 
        break;
      }
      comp_perf = width * ptt_val;
      if (comp_perf < shortest_exec) {
        shortest_exec = comp_perf;
        new_width = width;
        new_leader = leader;      
      }
    }
  }  
  it->width = new_width; 
  it->leader = new_leader;
  return new_leader;
}
#endif
  
#ifdef WEIGHT_SCHED
int PolyTask::weight_sched(int nthread, PolyTask * it){
  int ndx=nthread;
  double current_bias=bias;
  double div = 1;
  double little = 0; //Inital little value
  double big = 0; //Inital little value

  //Find index based on width
  double index = 0;
  index = log2(it->width);
  
  //Look at predefined little and big threads
  //with current width as index
  little = it->get_timetable(LITTLE_INDEX,index);
  big = it->get_timetable(BIG_INDEX,index); 

  //If it has no recorded value on big, choose big
  if(!big){
    ndx=BIG_INDEX;
  //If it has no recorded value on little, choose little
  }else if(!little){
    ndx=LITTLE_INDEX;
  //Else, check if benifical to run on big or little
  }else{
    //If larger than current global value
    //Schedule on big cluster 
    //otherwise LITTLE
    div = little/big;
    if(div > current_bias){
      ndx=BIG_INDEX;
    }else{
      ndx=LITTLE_INDEX;
    }
    //Update the bias with a ratio of 1:6 to the old bias
    current_bias=((6*current_bias)+div)/7;
    bias.store(current_bias);
  }
  
  //Choose a random big or little for scheduling
  if(ndx == BIG_INDEX){
    ndx=((rand()%BIG_NTHREADS)+ndx);
  }else{
    ndx=((rand()%LITTLE_NTHREADS)+ndx);
  }
#if defined(CRIT_PERF_SCHED) 
  //Mold the task to a new width
  history_mold(ndx,it);
#endif
  return ndx;
} 
#endif


PolyTask * PolyTask::commit_and_wakeup(int _nthread){
  PolyTask *ret = nullptr;
  for(std::list<PolyTask *>::iterator it = out.begin(); it != out.end(); ++it){
    int refs = (*it)->refcount.fetch_sub(1);
    if(refs == 1){
#ifdef DEBUG
      LOCK_ACQUIRE(output_lck);
      std::cout << "[DEBUG] Task " << (*it)->taskid << " became ready" << std::endl;
      LOCK_RELEASE(output_lck);
#endif 
        
#if defined(CRIT_PERF_SCHED) || defined(WEIGHT_SCHED)

#if defined(CRIT_PERF_SCHED)
      int pr = if_prio(_nthread, (*it));
      if (pr == 1){
        globalsearch_PTT(_nthread, (*it));
#ifdef DEBUG
        LOCK_ACQUIRE(output_lck);
        std::cout <<"[DEBUG] Priority=1, task "<< (*it)->taskid <<" will run on thread "<< (*it)->leader << ", width become " << (*it)->width << std::endl;
        LOCK_RELEASE(output_lck);
#endif
        for(int i = (*it)->leader; i < (*it)->leader + (*it)->width; i++){
          LOCK_ACQUIRE(worker_assembly_lock[i]);
          worker_assembly_q[i].push_back((*it));
        }
        for(int i = (*it)->leader; i < (*it)->leader + (*it)->width; i++){
          LOCK_RELEASE(worker_assembly_lock[i]);
        }        
      }
      else{        
#ifdef DEBUG
        LOCK_ACQUIRE(output_lck);
        std::cout <<"[DEBUG] Priority=0, task "<< (*it)->taskid <<" is pushed to WSQ of thread "<< _nthread << std::endl;
        LOCK_RELEASE(output_lck);
#endif
        LOCK_ACQUIRE(worker_lock[_nthread]);
        worker_ready_q[_nthread].push_front(*it);
        LOCK_RELEASE(worker_lock[_nthread]);
      }
#elif defined(WEIGHT_SCHED)
      int ndx2 = weight_sched(_nthread, (*it));
      LOCK_ACQUIRE(worker_lock[ndx2]);
      worker_ready_q[ndx2].push_front(*it);
      LOCK_RELEASE(worker_lock[ndx2]);
#endif
                
#else
      if(!ret && (((*it)->affinity_queue == -1) || (((*it)->affinity_queue/(*it)->width) == (_nthread/(*it)->width)))){
        // history_mold(_nthread,(*it)); 
        ret = *it; // forward locally only if affinity matches
      }
      else{
        // otherwise insert into affinity queue, or in local queue
        int ndx = (*it)->affinity_queue;
        if((ndx == -1) || (((*it)->affinity_queue/(*it)->width) == (_nthread/(*it)->width)))
          ndx = _nthread;

        //history_mold(_nthread,(*it)); 
        
        // seems like we acquire and release the lock for each assembly. 
        // This is suboptimal, but given that TAO_STA makes the allocation
        // somewhat random it simpifies the implementation. In the case that
        // TAO_STA is not defined, we could optimize it, but is it worth?
        LOCK_ACQUIRE(worker_lock[ndx]);
        worker_ready_q[ndx].push_front(*it);
        LOCK_RELEASE(worker_lock[ndx]);
      } 
#endif
    }
  }
  task_completions[_nthread].tasks++;
  return ret;
}       

#include "poly_task.h"
#include <stdlib.h>
#if defined(DEBUG)
#include <iostream>
#endif
//extern int TABLEWIDTH;
extern int gotao_nthreads;
extern int wid[GOTAO_NTHREADS];
extern aligned_lock worker_lock[MAXTHREADS];
extern int critical_path;
extern std::list<PolyTask *> worker_ready_q[MAXTHREADS];
extern completions task_completions[MAXTHREADS];
extern completions task_pool[MAXTHREADS];
extern int TABLEWIDTH;
#if TX2
extern int pac0;
extern int pac1;
#endif
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
  int new_width=1; 
  double shortest_exec=100;
  double comp_perf = 0.0; 
  for(int i=0; i<TABLEWIDTH;i++){
  int width = wid[i];
#if TX2
  if(width > std::min(pac0, pac1)){
    int newcore = ((_nthread-pac0)%width)*width + pac0;
    comp_perf = (width*(it->get_timetable(newcore,i)));
  }
  else{
    comp_perf = (width*(it->get_timetable((_nthread-(_nthread%(width))),i)));
  }
#else
    comp_perf = (width*(it->get_timetable((_nthread-(_nthread%(width))),i)));
#endif
    if (comp_perf < shortest_exec){
      shortest_exec = comp_perf;
      new_width = width;
    }
  }
  it->width=new_width;
  return 0;   
} 
#endif

  //Recursive function assigning criticality
#if defined(CRIT_PERF_SCHED)
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
  // int prio = 0;
  // if((it->marker) == 1){
  //   prio=1;
  // } 
  // return prio;
  return (it->criticality);
} 
#endif
  
#if defined(CRIT_PERF_SCHED)
int PolyTask::globalsearch_PTT(int nthread, PolyTask * it){
  float shortest_exec=100;
  int new_width_index = 0;
  for(int i = 0; i < TABLEWIDTH; i++)
  {
    for(int j = gotao_nthreads % wid[i]; j < gotao_nthreads; j += wid[i])
    {
      float temp = wid[i] * (it->get_timetable(j,i));
      if(temp < shortest_exec)
      {
        shortest_exec = temp;
        nthread = j;
        new_width_index = i;
      }
    }
  }
  it->width = wid[new_width_index]; 
  return nthread;
}
  
  
//Find suitable thread for prio task
int PolyTask::find_thread(int nthread, PolyTask * it){
  //Inital thread is own
  int ndx = nthread;
  //Set inital comparison value hihg
  double shortest_exec=100;
  int index = 0;
  for(int k=0; k<TABLEWIDTH; k++){
    if(it->width == wid[k]){
      index = k;
      break;
    }
  }
#if TX2
  if(it->width <= std::min(pac0, pac1)){
    for(int k=0; k<gotao_nthreads; (k=k+(it->width))){
      double temp = (it->get_timetable(k,index));
      if(temp*(it->width)<shortest_exec){
        shortest_exec = temp;
        ndx=k;
      }
    }
  }
  else{
#else
    for(int k=0; k<gotao_nthreads; (k=k+(it->width))){
      double temp = (it->get_timetable(k,index));
      if(temp*(it->width)<shortest_exec){
        shortest_exec = temp;
        ndx=k;
      }
    }
#endif
#if TX2
  }
#endif
    return ndx;
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
      int ndx2 = _nthread;
#ifdef DEBUG
      std::cout <<"[DEBUG] Task "<< (*it)->taskid <<"run on thread "<< ndx2 <<"originally"<< std::endl;
#endif

#if defined(CRIT_PERF_SCHED)
      int pr = if_prio(_nthread, (*it));
      if (pr == 1){
        ndx2 = globalsearch_PTT(_nthread, (*it));
#ifdef DEBUG
        LOCK_ACQUIRE(output_lck);
        std::cout <<"[DEBUG] Priority=1, task "<< (*it)->taskid <<" will run on thread "<< ndx2 << ", width become " << (*it)->width << std::endl;
        LOCK_RELEASE(output_lck);
#endif
      }
      else{
#if TX2
        ndx2 = rand()%pac1+pac0;
#else
        ndx2=(rand()%gotao_nthreads);
#endif
        history_mold(ndx2,(*it));
#ifdef DEBUG
        LOCK_ACQUIRE(output_lck);
        std::cout <<"[DEBUG] Priority=0, task "<< (*it)->taskid <<" will run on thread "<< ndx2 << ", width become " << (*it)->width << std::endl;
        LOCK_RELEASE(output_lck);
#endif
      }
#elif defined(WEIGHT_SCHED)
      ndx2 = weight_sched(_nthread, (*it));
#endif
        
      LOCK_ACQUIRE(worker_lock[ndx2]);
      worker_ready_q[ndx2].push_front(*it);
      LOCK_RELEASE(worker_lock[ndx2]);
        
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

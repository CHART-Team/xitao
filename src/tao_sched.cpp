/* assembly_sched.cxx -- integrated work stealing with assembly scheduling */

#include "tao.h"
#include "config.h"
#include "debug_info.h"
#include <iostream>
#include <chrono>
#include <algorithm>
#include <vector>
#include <fstream>
#include <assert.h>
#include "xitao_workspace.h"
#include "queue_manager.h"
// #include "perf_model.h"
using namespace xitao;

int worker_loop(int nthread);

//! Allocates/deallocates the XiTAO's runtime resources. The size of the vector is equal to the number of available CPU cores. 
/*!
  \param affinity_control Set the usage per each cpu entry in the cpu_set_t
 */
void set_xitao_mask(cpu_set_t& user_affinity_setup) {
  if(!gotao_initialized) {
    resources_runtime_controlled = true;                                    // make this true, to refrain from using XITAO_MAXTHREADS anywhere
    int cpu_count = CPU_COUNT(&user_affinity_setup);
    runtime_resource_mapper.resize(cpu_count);
    int j = 0;
    for(int i = 0; i < XITAO_MAXTHREADS; ++i) {
      if(CPU_ISSET(i, &user_affinity_setup)) {
        runtime_resource_mapper[j++] = i;
      }
    }
    if(cpu_count < xitao_nthreads) std::cout << "Warning: only " << cpu_count << " physical cores available, whereas " << xitao_nthreads << " are requested!" << std::endl;      
  } else {
    std::cout << "Warning: unable to set XiTAO affinity. Runtime is already initialized. This call will be ignored" << std::endl;      
  }    
}

//! Initialize the XiTAO Runtime
/*!
  \param nthr is the number of XiTAO threads 
  \param thrb is the logical thread id offset from the physical core mapping
  \param nhwc is the number of hardware contexts
*/ 
void xitao_init_hw(int nthr, int thrb, int nhwc)
{ 
  if(gotao_initialized) {
    xitao_ptt::clear_layout_partitions();
  }

  if(nthr>=0) xitao_nthreads = nthr;
  else {    
    if(getenv("GOTAO_NTHREADS")) 
      xitao_nthreads = atoi(getenv("GOTAO_NTHREADS"));  
    else 
      xitao_nthreads = XITAO_MAXTHREADS;    
  }  
  if(xitao_nthreads > XITAO_MAXTHREADS) {
    if(!suppress_init_warnings) std::cout << "Fatal error: xitao_nthreads is greater\
     than XITAO_MAXTHREADS of " << XITAO_MAXTHREADS << ". Make sure XITAO_MAXTHREADS\
      environment variable is set properly" << std::endl;    
    exit(0);
  }  

  if(resources_runtime_controlled) { 
    if(xitao_nthreads != runtime_resource_mapper.size()) 
      xitao_nthreads = runtime_resource_mapper.size();
  }
  const char* layout_file = getenv("XITAO_LAYOUT_PATH");
  if(layout_file) {
    if(!resources_runtime_controlled) 
      xitao_ptt::read_layout_table(layout_file);
  } else {
    if(!resources_runtime_controlled) {
      if(!suppress_init_warnings) std::cout << "Warning: XITAO_LAYOUT_PATH is not set.\
       Default values for affinity and symmetric resource partitions will be used" << std::endl;    
      for(int i = 0; i < XITAO_MAXTHREADS; ++i) 
        static_resource_mapper[i] = i; 
    } 
    xitao_ptt::prepare_default_layout(xitao_nthreads);
  }
  if(nhwc>=0){
    gotao_ncontexts = nhwc;
  }
  else{
    if(getenv("GOTAO_HW_CONTEXTS")){
      gotao_ncontexts = atoi(getenv("GOTAO_HW_CONTEXTS"));
    }
    else{ 
      gotao_ncontexts = GOTAO_HW_CONTEXTS;
    }
  }

  if(thrb>=0){
    gotao_thread_base = thrb;
  }
  else{
    if(getenv("GOTAO_THREAD_BASE")){
      gotao_thread_base = atoi(getenv("GOTAO_THREAD_BASE"));
    }
    else{
      gotao_thread_base = GOTAO_THREAD_BASE;
    }
  }
  xitao_ptt::print_ptt_debug_info();
  suppress_init_warnings = true;    
  gotao_initialized = true;  
}


void xitao_set_num_threads(int nthreads) {
  config::nthreads = nthreads;
}

// Initialize gotao from environment vars or defaults
void xitao_init() {
  xitao_init_hw(config::nthreads, config::thread_base, config::hw_contexts);
  config::print_configs();
}

void xitao_init(int argc, char** argv) { 
  config::init_config(argc, argv);
  xitao_init_hw(config::nthreads, config::thread_base, config::hw_contexts);
  config::print_configs();
}

void xitao_start()
{
 if(gotao_started) return;
  starting_barrier = new BARRIER(xitao_nthreads + 1);
  tao_barrier = new cxx_barrier(2);
  for(int i = 0; i < xitao_nthreads; i++){
    t[i]  = new std::thread(worker_loop, i);   
  }  
  starting_barrier->wait();
  gotao_started = true;
}

void xitao_fini() {
  resources_runtime_controlled = false;
  gotao_can_exit = true;
  gotao_started = false;
  //gotao_initialized = false;
  tao_total_steals = 0;  
  for(int i = 0; i < xitao_nthreads; i++){
    t[i]->join();
  }
}

// drain the pipeline
void xitao_drain()
{
  std::unique_lock<std::mutex> lk(pending_tasks_mutex);
  pending_tasks_cv.wait(lk, []{return PolyTask::pending_tasks <= 0;});
}

// a way to force sync between master and TAOs
// usage of gotao_barrier() is not recommended
void xitao_barrier()
{
  tao_barrier->wait();
}

int check_and_get_available_queue(int queue) {
  bool found = false;
  if(queue >= runtime_resource_mapper.size()) {
    return rand()%runtime_resource_mapper.size();
  } else {
    return queue;
  }  
}
// push work into polytask queue
// if no particular queue is specified then try to determine which is the local
// queue and insert it there. This has some overhead, so in general the
// programmer should specify some queue
void xitao_push(PolyTask *pt, int queue)
{
  if((queue == -1) && (pt->affinity_queue != -1) && config::sta){
    queue = pt->affinity_queue;
  }
  else{
    if(queue == -1){
      queue = sched_getcpu();
    }
  }
  if(resources_runtime_controlled) {
    queue = check_and_get_available_queue(queue);
  } else { // check if the insertion happens in invalid queue (according to PTT layout)
    while(inclusive_partitions[queue].size() == 0){
      queue = (queue + 1) % xitao_nthreads;
    }
  }
  if(config::use_performance_modeling)
    pt->_ptt = xitao_ptt::try_insert_table(pt, pt->workload_hint);    /*be sure that a single orphaned task has a PTT*/
  default_queue_manager::insert_in_ready_queue(pt, queue);
}

// Push work when not yet running. This version does not require locks
// Semantics are slightly different here
// 1. the tid refers to the logical core, before adjusting with gotao_thread_base
// 2. if the queue is not specified, then put everything into the first queue
int gotao_push_init(PolyTask *pt, int queue)
{
  if((queue == -1) && (pt->affinity_queue != -1)){
    queue = pt->affinity_queue;
  }
  else{
    if(queue == -1){
      queue = gotao_thread_base;
    }
  }
  if(resources_runtime_controlled) queue = check_and_get_available_queue(queue);
  default_queue_manager::insert_in_ready_queue(pt, queue);
  return 1; 
}

void __xitao_lock()
{
  smpd_region_lock.lock();
  //LOCK_ACQUIRE(smpd_region_lock);
}
void __xitao_unlock()
{
  smpd_region_lock.unlock();
  //LOCK_RELEASE(smpd_region_lock);
}

int worker_loop(int nthread)
{
  int phys_core;
  if(resources_runtime_controlled) {
    if(nthread >= runtime_resource_mapper.size()) {      
      std::cout << "Error: thread cannot be created due to resource limitation" << std::endl;      
      exit(0);
    }
    phys_core = runtime_resource_mapper[nthread];
  } else {
    phys_core = static_resource_mapper[gotao_thread_base+(nthread%(XITAO_MAXTHREADS-gotao_thread_base))];   
  }  
  DEBUG_MSG("[DEBUG] nthread: " << nthread << " mapped to physical core: "<< phys_core); 
  unsigned int seed = time(NULL);
  cpu_set_t cpu_mask;
  CPU_ZERO(&cpu_mask);
  CPU_SET(phys_core, &cpu_mask);

  sched_setaffinity(0, sizeof(cpu_set_t), &cpu_mask); 
  // When resources are reclaimed, this will preempt the thread if it has no work in its local queue to do.
  
  PolyTask *st = nullptr;
  starting_barrier->wait();  
  auto&&  partitions = inclusive_partitions[nthread];
  auto&& largest_inclusive_partition = inclusive_partitions[nthread].back();

  if(partitions.size() == 0) {
    std::cout << "Thread " << nthread << " is deactivated since it is not included in any PTT partition" << std::endl;      
    return 0;
  }
  while(true) {    
    int random_core = 0;
    AssemblyTask *assembly = nullptr;
    SimpleTask *simple = nullptr;

    // 0. If a task is already provided via forwarding then exeucute it (simple task)
    //    or insert it into the assembly queues (assembly task)
    if(st) {
      if(st->type == TASK_SIMPLE){
        SimpleTask *simple = (SimpleTask *) st;
        simple->f(simple->args, nthread);
        DEBUG_MSG("[DEBUG] Distributing simple task " << simple->taskid << " with width " << simple->width << " to workers " << nthread);
        st = simple->commit_and_wakeup(nthread);
        simple->cleanup();
        //delete simple;
      }
      else if(st->type == TASK_ASSEMBLY){
        AssemblyTask *assembly = (AssemblyTask *) st;
        // defensive check against uninitialized leader
        if(assembly->leader < 0) {
          assembly->leader = nthread / assembly->width * assembly->width; 
        }       
        default_queue_manager::insert_task_in_assembly_queues(st);
        st = nullptr;
      }
      continue;
    }

  // 1. check for assemblies
    if(!default_queue_manager::try_pop_assembly_task(nthread, st)){
      st = nullptr;
    }
  // assemblies are inlined between two barriers
    if(st) {
      bool _final; // remaining
      assembly = (AssemblyTask *) st;
      std::chrono::time_point<std::chrono::system_clock> t1,t2; 
      if(config::use_performance_modeling) {
        if(assembly->leader == nthread){
          t1 = std::chrono::system_clock::now();
        }
      }
      DEBUG_MSG("[DEBUG] Thread "<< nthread << " starts executing task " << assembly->taskid << "......");
#ifdef NEED_BARRIER
      assembly->threads_in_tao++;
      while(assembly->threads_in_tao < assembly->width);
#endif      
      assembly->execute(nthread);

      if(config::use_performance_modeling && assembly->leader == nthread) {
        t2 = std::chrono::system_clock::now();
        std::chrono::duration<double> elapsed_seconds = t2-t1;
        double ticks = elapsed_seconds.count();
        perf_model::update_performance_model(assembly, nthread, ticks, assembly->width);
        //assembly->insert_modeled_performance(nthread, ticks, width_index);
      }

      _final = (++assembly->threads_out_tao == assembly->width);
      st = nullptr;
      if(_final) { // the last exiting thread updates
        DEBUG_MSG("[DEBUG] Thread " << nthread << " completed assembly task " << assembly->taskid);
        assembly->cleanup();
        st = assembly->commit_and_wakeup(nthread);
        if(config::delete_executed_taos) delete assembly;
      }
      continue;
    }

    // 2. check own queue: if a ready local task is found continue,
    //  to avoid trying to steal
    if(default_queue_manager::try_pop_ready_task(nthread, st, nthread)) continue;

    // 3. try to steal 
    // STEAL_ATTEMPTS determines number of steals before retrying the loop
    if(config::enable_workstealing && config::steal_attempts && !(rand_r(&seed) % config::steal_attempts)) {
      int attempts = 1;
      do{
	int steal_tries = 0;
        bool is_local_steal = false;
        do{
          if(config::enable_local_workstealing) { 
	    steal_tries++;
	    is_local_steal = true;
	    random_core = largest_inclusive_partition.first + (rand_r(&seed)%largest_inclusive_partition.second);
	  } else {
	    random_core = (rand_r(&seed) % xitao_nthreads);
	  } 
      } while(random_core == nthread && steal_tries < 2);
          if(default_queue_manager::try_pop_ready_task(random_core, st, nthread)) {
            tao_total_steals++;
	  }
      } while(!st && (attempts-- > 0));
      if(st){
        continue;
      }
    } 

    // 4. It may be that there are no more tasks in the flow
    // this condition signals termination of the program
    // First check the number of actual tasks that have completed
    if(task_completions[nthread].tasks > 0){
      PolyTask::pending_tasks -= task_completions[nthread].tasks;
      DEBUG_MSG("[DEBUG] Thread " << nthread << " completed " << task_completions[nthread].tasks << " tasks. " <<
      "Pending tasks = " << PolyTask::pending_tasks);
      task_completions[nthread].tasks = 0;
    }
    LOCK_ACQUIRE(worker_lock[nthread]);
   // Next remove any virtual tasks from the per-thread task pool
    if(task_pool[nthread].tasks > 0) {
      PolyTask::pending_tasks -= task_pool[nthread].tasks;
      DEBUG_MSG("[DEBUG] Thread " << nthread << " removed " << task_pool[nthread].tasks << " virtual tasks. " <<
      "Pending tasks = " << PolyTask::pending_tasks);
      task_pool[nthread].tasks = 0;
    }
    LOCK_RELEASE(worker_lock[nthread]);
    // Finally check if the program has terminated
    if(PolyTask::pending_tasks == 0) pending_tasks_cv.notify_one();
    if(gotao_can_exit && (PolyTask::pending_tasks == 0)){
      break;
    }
  }
  return 0;
}

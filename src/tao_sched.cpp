/* assembly_sched.cxx -- integrated work stealing with assembly scheduling */

#include "tao.h"
// #include "debug_info.h"
#include <iostream>
#include <chrono>
#include <cmath>
#include <algorithm>
#include <vector>
#include <fstream>

// define the topology
int gotao_sys_topo[5] = TOPOLOGY;

// a PolyTask is either an assembly or a simple task
std::list<PolyTask *> worker_ready_q[XITAO_MAXTHREADS];
LFQueue<PolyTask *> worker_assembly_q[XITAO_MAXTHREADS];
GENERIC_LOCK(worker_assembly_lock[XITAO_MAXTHREADS]);
long int tao_total_steals = 0;
GENERIC_LOCK(worker_lock[XITAO_MAXTHREADS]);
GENERIC_LOCK(output_lck);
BARRIER *starting_barrier;
cxx_barrier *tao_barrier;
//int wid[XITAO_MAXTHREADS] = {1};
std::vector<int>                       static_resource_mapper(XITAO_MAXTHREADS);
std::vector<std::vector<int> >     ptt_layout(XITAO_MAXTHREADS);
std::vector<std::vector<std::pair<int, int> > > inclusive_partitions(XITAO_MAXTHREADS);

struct completions task_completions[XITAO_MAXTHREADS];
struct completions task_pool[XITAO_MAXTHREADS];
cpu_set_t affinity_setup;
int critical_path;
int gotao_nthreads;
int gotao_ncontexts;
int gotao_thread_base;
bool gotao_can_exit = false;
bool gotao_initialized = false;
bool resources_runtime_conrolled = false;
std::vector<int> runtime_resource_mapper;                                   // a logical to physical runtime resource mapper
int TABLEWIDTH;
int worker_loop(int);
std::thread *t[XITAO_MAXTHREADS];

// std::vector<thread_info> thread_info_vector(XITAO_MAXTHREADS);
//! Allocates/deallocates the XiTAO's runtime resources. The size of the vector is equal to the number of available CPU cores. 
/*!
  \param affinity_control Set the usage per each cpu entry in the cpu_set_t
 */
int set_xitao_mask(cpu_set_t& user_affinity_setup) {
  if(!gotao_initialized) {
    resources_runtime_conrolled = true;                                    // make this true, to refrain from using XITAO_MAXTHREADS anywhere
    int cpu_count = CPU_COUNT(&user_affinity_setup);
    runtime_resource_mapper.resize(cpu_count);
    int j = 0;
    for(int i = 0; i < XITAO_MAXTHREADS; ++i) {
      if(CPU_ISSET(i, &user_affinity_setup)) {
        runtime_resource_mapper[j++] = i;
      }
    }
    if(cpu_count < gotao_nthreads) std::cout << "Warning: only " << cpu_count << " physical cores available, whereas " << gotao_nthreads << " are requested!" << std::endl;      
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
int gotao_init_hw( int nthr, int thrb, int nhwc)
{
  gotao_initialized = true;
  if(nthr>=0) gotao_nthreads = nthr;
  else {    
    if(getenv("GOTAO_NTHREADS")) gotao_nthreads = atoi(getenv("GOTAO_NTHREADS"));  
    else gotao_nthreads = XITAO_MAXTHREADS;    
  }  
  if(gotao_nthreads > XITAO_MAXTHREADS) {
    std::cout << "Fatal error: gotao_nthreads is greater than XITAO_MAXTHREADS of " << XITAO_MAXTHREADS << ". Make sure XITAO_MAXTHREADS environment variable is set properly" << std::endl;    
    exit(0);
  }  
  const char* layout_file = getenv("XITAO_LAYOUT_PATH");
  if(!resources_runtime_conrolled) {
    if(layout_file) {
      std::string line;      
      std::ifstream myfile(layout_file);
      int current_thread_id = -1; // exclude the first iteration
      if (myfile.is_open()) {
        bool init_affinity = false;
        while (std::getline(myfile,line)) {          
          size_t pos = 0;
          std::string token;
          if(current_thread_id >= XITAO_MAXTHREADS) {
            std::cout << "Fatal error: there are more partitions than XITAO_MAXTHREADS of: " << XITAO_MAXTHREADS  << " in file: " << layout_file << std::endl;    
            exit(0);    
          }          
          int thread_count = 0;
          while ((pos = line.find(",")) != std::string::npos) {
            token = line.substr(0, pos);      
            int val = stoi(token);
            if(!init_affinity) static_resource_mapper[thread_count++] = val;  
            else { 
              if(current_thread_id + 1 >= gotao_nthreads) {
                  std::cout << "Fatal error: more configurations than there are input threads in:" << layout_file << std::endl;    
                  exit(0);
              }
              ptt_layout[current_thread_id].push_back(val);
              for(int i = 0; i < val; ++i) {     
                if(current_thread_id + i >= XITAO_MAXTHREADS) {
                  std::cout << "Fatal error: illegal partition choices for thread: " << current_thread_id <<" spanning id: " << current_thread_id + i << " while having XITAO_MAXTHREADS: " << XITAO_MAXTHREADS  << " in file: " << layout_file << std::endl;    
                  exit(0);           
                }
                inclusive_partitions[current_thread_id + i].push_back(std::make_pair(current_thread_id, val)); 
              }              
            }            
            line.erase(0, pos + 1);
          }          
          if(line.size() > 0) {
            token = line.substr(0, line.size());      
            int val = stoi(token);
            if(!init_affinity) static_resource_mapper[thread_count++] = val;
            else { 
              ptt_layout[current_thread_id].push_back(val);
              for(int i = 0; i < val; ++i) {                
                if(current_thread_id + i >= XITAO_MAXTHREADS) {
                  std::cout << "Fatal error: illegal partition choices for thread: " << current_thread_id <<" spanning id: " << current_thread_id + i << " while having XITAO_MAXTHREADS: " << XITAO_MAXTHREADS  << " in file: " << layout_file << std::endl;    
                  exit(0);           
                }
                inclusive_partitions[current_thread_id + i].push_back(std::make_pair(current_thread_id, val)); 
              }              
            }            
          }
          if(!init_affinity) { 
            gotao_nthreads = thread_count; 
            init_affinity = true;
          }
          current_thread_id++;          
        }
        myfile.close();
      } else {
        std::cout << "Fatal error: could not open hardware layout path " << layout_file << std::endl;    
        exit(0);
      }
    } else {
        std::cout << "Warning: XITAO_LAYOUT_PATH is not set. Default values for affinity and symmetric resoruce partitions will be used" << std::endl;    
        for(int i = 0; i < XITAO_MAXTHREADS; ++i) 
          static_resource_mapper[i] = i; 
        std::vector<int> widths;             
        int count = gotao_nthreads;        
        std::vector<int> temp;        // hold the big divisors, so that the final list of widths is in sorted order 
        for(int i = 1; i < sqrt(gotao_nthreads); ++i){ 
          if(gotao_nthreads % i == 0) {
            widths.push_back(i);
            temp.push_back(gotao_nthreads / i); 
          } 
        }
        widths.insert(widths.end(), temp.begin(), temp.end());
        //std::reverse(widths.begin(), widths.end());        
        for(int i = 0; i < widths.size(); ++i) {
          for(int j = 0; j < gotao_nthreads; j+=widths[i]){
            ptt_layout[j].push_back(widths[i]);
          }
        }
        for(int i = 0; i < gotao_nthreads; ++i){
          for(auto&& width : ptt_layout[i]){
            for(int j = 0; j < width; ++j) {                
              inclusive_partitions[i + j].push_back(std::make_pair(i, width)); 
            }         
          }
        }
      } 
  } else {    
    if(gotao_nthreads != runtime_resource_mapper.size()) {
      std::cout << "Warning: requested " << runtime_resource_mapper.size() << " at runtime, whereas gotao_nthreads is set to " << gotao_nthreads <<". Runtime value will be used" << std::endl;
      gotao_nthreads = runtime_resource_mapper.size();
    }            
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
  starting_barrier = new BARRIER(gotao_nthreads + 1);
  tao_barrier = new cxx_barrier(2);
  for(int i = 0; i < gotao_nthreads; i++){
    t[i]  = new std::thread(worker_loop, i);   
  }  
  std::cout << "XiTAO initialized with " << gotao_nthreads << " threads and configured with " << XITAO_MAXTHREADS << " max threads " << std::endl;
#ifdef DEBUG
  for(int i = 0; i < static_resource_mapper.size(); ++i) { 
    std::cout << "[DEBUG] thread " << i << " is configured to be mapped to core id : " << static_resource_mapper[i] << std::endl;     
    std::cout << "[DEBUG] leader thread " << i << " has partition widths of : "; 
    for (int j = 0; j < ptt_layout[i].size(); ++j){
      std::cout << ptt_layout[i][j] << " ";      
    }
    std::cout << std::endl;
    std::cout << "[DEBUG] thread " << i << " is contained in these [leader,width] pairs : ";
    for (int j = 0; j < inclusive_partitions[i].size(); ++j){
      std::cout << "["<<inclusive_partitions[i][j].first << "," << inclusive_partitions[i][j].second << "]"; 
    }
    std::cout << std::endl;
  }
#endif    
}

// Initialize gotao from environment vars or defaults
int gotao_init()
{
  return gotao_init_hw(-1, -1, -1);
}

int gotao_start()
{
#ifdef WEIGHT_SCHED
  //Store inital weight value
  PolyTask::bias.store(1.5);
#endif
  
#if defined(CRIT_PERF_SCHED) 
  //Analyse DAG based on tasks in ready q and asign criticality values
  for(int j=0; j<gotao_nthreads; j++){
    //Iterate over all ready tasks for all threads
    for(std::list<PolyTask *>::iterator it = worker_ready_q[j].begin(); it != worker_ready_q[j].end(); ++it){
      //Call recursive function setting criticality
      (*it)->set_criticality();
    }
  }
  for(int j = 0; j < gotao_nthreads; j++){
    for(std::list<PolyTask *>::iterator it = worker_ready_q[j].begin(); it != worker_ready_q[j].end(); ++it){
      if ((*it)->criticality == critical_path){
        (*it)->marker = 1;
        (*it) -> set_marker(1);
        break;
      }
    }
  }
#endif
  starting_barrier->wait();
}

int gotao_fini()
{
  resources_runtime_conrolled = false;
  gotao_can_exit = true;
  gotao_initialized = false;
  for(int i = 0; i < gotao_nthreads; i++){
    t[i]->join();
  }
}

void gotao_barrier()
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
int gotao_push(PolyTask *pt, int queue)
{
  if((queue == -1) && (pt->affinity_queue != -1)){
    queue = pt->affinity_queue;
  }
  else{
    if(queue == -1){
      queue = sched_getcpu();
    }
  }
  if(resources_runtime_conrolled) queue = check_and_get_available_queue(queue);
  LOCK_ACQUIRE(worker_lock[queue]);
  worker_ready_q[queue].push_front(pt);
  LOCK_RELEASE(worker_lock[queue]);
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
  if(resources_runtime_conrolled) queue = check_and_get_available_queue(queue);
  worker_ready_q[queue].push_front(pt);
}

// alternative version that pushes to the back
int gotao_push_back_init(PolyTask *pt, int queue)
{
  if((queue == -1) && (pt->affinity_queue != -1)){
    queue = pt->affinity_queue;
  }
  else{
    if(queue == -1){
      queue = gotao_thread_base;
    }
  }
  worker_ready_q[queue].push_back(pt);
}


long int r_rand(long int *s)
{
  *s = ((1140671485*(*s) + 12820163) % (1<<24));
  return *s;
}

int worker_loop(int nthread)
{
  int phys_core;
  if(resources_runtime_conrolled) {
    if(nthread >= runtime_resource_mapper.size()) {
      LOCK_ACQUIRE(output_lck);
      std::cout << "Error: thread cannot be created due to resource limitation" << std::endl;
      LOCK_RELEASE(output_lck);
      exit(0);
    }
    phys_core = runtime_resource_mapper[nthread];
  } else {
    phys_core = static_resource_mapper[gotao_thread_base+(nthread%(XITAO_MAXTHREADS-gotao_thread_base))];   
  }
#ifdef DEBUG
  LOCK_ACQUIRE(output_lck);
  std::cout << "[DEBUG] nthread: " << nthread << " mapped to physical core: "<< phys_core << std::endl;
  LOCK_RELEASE(output_lck);
#endif  
  unsigned int seed = time(NULL);
  cpu_set_t cpu_mask;
  CPU_ZERO(&cpu_mask);
  CPU_SET(phys_core, &cpu_mask);

  sched_setaffinity(0, sizeof(cpu_set_t), &cpu_mask); 
  // When resources are reclaimed, this will preempt the thread if it has no work in its local queue to do.
  
  PolyTask *st = nullptr;
  starting_barrier->wait();  
  auto&&  partitions = inclusive_partitions[nthread];
  while(true)
  {    
    int random_core = 0;
    AssemblyTask *assembly = nullptr;
    SimpleTask *simple = nullptr;

  // 0. If a task is already provided via forwarding then exeucute it (simple task)
  //    or insert it into the assembly queues (assembly task)
    if(st){
      if(st->type == TASK_SIMPLE){
        SimpleTask *simple = (SimpleTask *) st;
        simple->f(simple->args, nthread);

  #ifdef DEBUG
        LOCK_ACQUIRE(output_lck);
        std::cout << "[DEBUG] Distributing simple task " << simple->taskid << " with width " << simple->width << " to workers " << nthread << std::endl;
        LOCK_RELEASE(output_lck);
  #endif
        st = simple->commit_and_wakeup(nthread);
        simple->cleanup();
        //delete simple;
      }
      else if(st->type == TASK_ASSEMBLY){
        AssemblyTask *assembly = (AssemblyTask *) st;
#ifndef CRIT_PERF_SCHED 
        assembly->leader = nthread / assembly->width * assembly->width; // homogenous calculation of leader core
#endif        
#ifdef DEBUG
        LOCK_ACQUIRE(output_lck);
        std::cout << "[DEBUG] Distributing assembly task " << assembly->taskid << " with width " << assembly->width << " to workers [" << assembly->leader << "," << assembly->leader + assembly->width << ")" << std::endl;
        LOCK_RELEASE(output_lck);
#endif
        for(int i = assembly->leader; i < assembly->leader + assembly->width; i++){
          LOCK_ACQUIRE(worker_assembly_lock[i]);
          worker_assembly_q[i].push_back(st);
        }
        for(int i = assembly->leader; i < assembly->leader + assembly->width; i++){
          LOCK_RELEASE(worker_assembly_lock[i]);
        }
        st = nullptr;
      }
      continue;
    }

  // 1. check for assemblies
    if(!worker_assembly_q[nthread].pop_front(&st)){
      st = nullptr;
    }
  // assemblies are inlined between two barriers
    if(st) {
      int _final; // remaining
      assembly = (AssemblyTask *) st;

#if defined(CRIT_PERF_SCHED)
      std::chrono::time_point<std::chrono::system_clock> t1,t2; 
      if(assembly->leader == nthread){
        t1 = std::chrono::system_clock::now();
      }
#endif

#ifdef DEBUG
      LOCK_ACQUIRE(output_lck);
      std::cout << "[DEBUG] Thread "<< nthread << " starts executing task " << assembly->taskid << "......\n";
      LOCK_RELEASE(output_lck);
#endif
      assembly->execute(nthread);

#if defined(CRIT_PERF_SCHED)
      if(assembly->leader == nthread){
        t2 = std::chrono::system_clock::now();
        std::chrono::duration<double> elapsed_seconds = t2-t1;
        double ticks = elapsed_seconds.count();
        int width_index = assembly->width - 1;
        //Weight the newly recorded ticks to the old ticks 1:4 and save
        float oldticks = assembly->get_timetable( nthread,width_index);
        if(oldticks == 0){
          assembly->set_timetable(nthread,ticks,width_index);
        }
        else {
          assembly->set_timetable(nthread,((4*oldticks+ticks)/5),width_index);         
        }
    }
#endif
    _final = (++assembly->threads_out_tao == assembly->width);
     st = nullptr;
     if(_final){ // the last exiting thread updates
#ifdef DEBUG
        LOCK_ACQUIRE(output_lck);
        std::cout << "[DEBUG] Thread " << nthread << " completed assembly task " << assembly->taskid << std::endl;
        LOCK_RELEASE(output_lck);
#endif
        st = assembly->commit_and_wakeup(nthread);
        assembly->cleanup();
        //delete assembly;
      }
      continue;
    }

  // 2. check own queue
    LOCK_ACQUIRE(worker_lock[nthread]);
    if(!worker_ready_q[nthread].empty()){
      st = worker_ready_q[nthread].front(); 
      worker_ready_q[nthread].pop_front();
      LOCK_RELEASE(worker_lock[nthread]);
#if defined(CRIT_PERF_SCHED)
      st->history_mold(nthread, st);
#endif      
#ifdef DEBUG
      LOCK_ACQUIRE(output_lck);
      std::cout <<"[DEBUG] Priority=0, task "<< st->taskid <<" will run on thread "<< st->leader << ", width become " << st->width << std::endl;
      LOCK_RELEASE(output_lck);
#endif
      continue;
    }     
    LOCK_RELEASE(worker_lock[nthread]);        
  // 3. try to steal 
  // TAO_WIDTH determines which threads participates in stealing
  // STEAL_ATTEMPTS determines number of steals before retrying the loop
    if(STEAL_ATTEMPTS && !(rand_r(&seed) % STEAL_ATTEMPTS)){
      int attempts = 1;
      do{
        do{
          random_core = (rand_r(&seed) % gotao_nthreads);
        } while(random_core == nthread);

        LOCK_ACQUIRE(worker_lock[random_core]);
        if(!worker_ready_q[random_core].empty()){
          st = worker_ready_q[random_core].back(); 
          worker_ready_q[random_core].pop_back();
          tao_total_steals++;  
    #ifdef DEBUG
          LOCK_ACQUIRE(output_lck);
          std::cout << "[DEBUG] Thread " << nthread << " steal a task from " << random_core << " successfully. \n";
          LOCK_RELEASE(output_lck);          
    #endif     
#if defined(CRIT_PERF_SCHED)          
          st->history_mold(nthread, st);     
#endif          
        }
        LOCK_RELEASE(worker_lock[random_core]);  
      }while(!st && (attempts-- > 0));
      if(st){
        continue;
      }
    } 

    // 4. It may be that there are no more tasks in the flow
    // this condition signals termination of the program
    // First check the number of actual tasks that have completed
    if(task_completions[nthread].tasks > 0){
      PolyTask::pending_tasks -= task_completions[nthread].tasks;
#ifdef DEBUG
      LOCK_ACQUIRE(output_lck);
      std::cout << "[DEBUG] Thread " << nthread << " completed " << task_completions[nthread].tasks << " tasks. " 
      "Pending tasks = " << PolyTask::pending_tasks << "\n";
      LOCK_RELEASE(output_lck);
#endif

      task_completions[nthread].tasks = 0;
    }
    LOCK_ACQUIRE(worker_lock[nthread]);
   // Next remove any virtual tasks from the per-thread task pool
    if(task_pool[nthread].tasks > 0){
      PolyTask::pending_tasks -= task_pool[nthread].tasks;
#ifdef DEBUG
      LOCK_ACQUIRE(output_lck);
      std::cout << "[DEBUG] Thread " << nthread << " removed " << task_pool[nthread].tasks << " virtual tasks. " 
      "Pending tasks = " << PolyTask::pending_tasks << "\n";
      LOCK_RELEASE(output_lck);
#endif
      task_pool[nthread].tasks = 0;
    }
    LOCK_RELEASE(worker_lock[nthread]);
    // Finally check if the program has terminated
    if(gotao_can_exit && (PolyTask::pending_tasks == 0)){
      break;
    }
  }
  return 0;
}

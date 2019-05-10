/* assembly_sched.cxx -- integrated work stealing with assembly scheduling */

#include "tao.h"
#include <iostream>
#include <chrono>
#include <cmath>
#include <vector>

// define the topology
int gotao_sys_topo[5] = TOPOLOGY;

// a PolyTask is either an assembly or a simple task
std::list<PolyTask *> worker_ready_q[MAXTHREADS];
GENERIC_LOCK(worker_lock[MAXTHREADS]);
GENERIC_LOCK(output_lck);
BARRIER *starting_barrier;
cxx_barrier *tao_barrier;
int wid[GOTAO_NTHREADS] = {1};
std::vector<int> PHYSCORE(MAXTHREADS);
struct completions task_completions[MAXTHREADS];
struct completions task_pool[MAXTHREADS];

#if TX2
int pac0;
int pac1;
#endif
int critical_path;
int gotao_nthreads;
int gotao_ncontexts;
int gotao_thread_base;
bool gotao_can_exit = false;
int TABLEWIDTH;
int worker_loop(int);
std::thread *t[MAXTHREADS];

//! Initialize the XiTAO Runtime
/*!
  \param nthr is the number of XiTAO threads 
  \param thrb is the logical thread id offset from the physical core mapping
  \param nhwc is the number of hardware contexts
*/ 
int gotao_init_hw( int nthr, int thrb, int nhwc)
{
  const char* affinity = getenv("XITAO_AFFINITY");
  if(affinity) {    
    std::string s(affinity);
    size_t pos = 0;
    std::string token;
    int i = 0; 
    s.erase(0, pos + 1);
    while ((pos = s.find(",")) != std::string::npos && i < MAXTHREADS) {
      token = s.substr(0, pos);      
      PHYSCORE[i++] = stoi(token);
      s.erase(0, pos + 1);
    }
  } else { 
    std::cout << "Warning: affinity not set. To set it, use export XITAO_AFFINITY =\"[int,[int,...]]\"" << std::endl;
    for(int i = 0; i < MAXTHREADS; ++i)
      PHYSCORE[i] = i;
  } 
#if TX2
  if(nthr>=0){
    pac0 = nthr; 
    pac1 = nthr;
  }
  else{ 
    if(getenv("PAC0")){
      pac0 = atoi(getenv("PAC0"));
    }
    else{
      pac0 = PAC0;
    }
    if(getenv("PAC1")){
      pac1 = atoi(getenv("PAC1"));
    }
    else{
      pac1 = PAC1;
    }
  }
  gotao_nthreads = pac0 + pac1;
#else
  if(nthr>=0) gotao_nthreads = nthr;
  else {    
    if(getenv("GOTAO_NTHREADS")) gotao_nthreads = atoi(getenv("GOTAO_NTHREADS"));
    else gotao_nthreads = GOTAO_NTHREADS;
  }
#endif
  if(gotao_nthreads > MAXTHREADS) {
    std::cout << "Fatal error: gotao_nthreads is greater than MAXTHREADS of " << MAXTHREADS << ". Make sure GOTAO_NTHREADS environment variable is set properly" << std::endl;    
    exit(0);
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
#if TX2
  TABLEWIDTH = 1;
  int a = 1;
  for(int i = 2; i <= std::max(pac0,pac1); i++){
    if((pac0 % i == 0) || (pac1 % i == 0)){
      TABLEWIDTH++;
      wid[a] = i;
      a++;  
    }
  }
#else
  TABLEWIDTH = 2;
  int a = 1;
  for(int i = 2; i < gotao_nthreads; i++){
    if((gotao_nthreads % i == 0)){
      TABLEWIDTH++;
    }
  }

  for(int i = 2; i < gotao_nthreads; i++){
    if((gotao_nthreads % i == 0)){
      wid[a] = i;
      a++;  
    }
  }
  wid[a] = gotao_nthreads;
#endif
#ifdef DEBUG
  LOCK_ACQUIRE(output_lck);
  std::cout<<"[DEBUG] Width length = "<<TABLEWIDTH<<". Width: ";
  for(int i = 0; i < TABLEWIDTH; i++)
  {
    std::cout<<wid[i]<<"  ";
  }
  std::cout<< "\n";
  LOCK_RELEASE(output_lck);
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
  gotao_can_exit = true;
  for(int i = 0; i < gotao_nthreads; i++){
    t[i]->join();
  }
}

void gotao_barrier()
{
  tao_barrier->wait();
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

LFQueue<PolyTask *> worker_assembly_q[MAXTHREADS];
GENERIC_LOCK(worker_assembly_lock[MAXTHREADS]);
long int tao_total_steals = 0;
long int r_rand(long int *s)
{
  *s = ((1140671485*(*s) + 12820163) % (1<<24));
  return *s;
}

int worker_loop(int nthread)
{
  int phys_core = PHYSCORE[gotao_thread_base+(nthread%(MAXTHREADS-gotao_thread_base))];   
#ifdef DEBUG
  LOCK_ACQUIRE(output_lck);
  std::cout << "[DEBUG] nthread: " << nthread << " mapped to physical core: "<< phys_core << std::endl;
  LOCK_RELEASE(output_lck);
#endif

  long int seed = 123456789;

  cpu_set_t cpu_mask;
  CPU_ZERO(&cpu_mask);
  CPU_SET(phys_core, &cpu_mask);

  sched_setaffinity(0, sizeof(cpu_set_t), &cpu_mask); 

  PolyTask *st = nullptr;
  starting_barrier->wait();

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
#if TX2       
        if((assembly->width <= std::min(pac0,pac1)) && ((std::min(pac0,pac1)) % assembly->width == 0) ){
          assembly->leader = ( nthread / assembly->width) * assembly->width;
        }
        else{ 
          if(nthread < pac0){
            assembly->leader = pac0;
          } 
          else{
            assembly->leader = pac0 + ( (nthread-pac0) / assembly->width) * assembly->width;
          }
        }
#else 
        int leader = ( nthread / assembly->width) * assembly->width;
        assembly->leader = leader;
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
    if(st){
int _final; // remaining
assembly = (AssemblyTask *) st;

#if defined(CRIT_PERF_SCHED)
std::chrono::time_point<std::chrono::system_clock> t1,t2; 
if(!(nthread-(assembly->leader))){
  t1 = std::chrono::system_clock::now();
}
#endif

#ifdef DEBUG
LOCK_ACQUIRE(output_lck);
std::cout << "[DEBUG] Thread "<< nthread << " start executing task " << assembly->taskid << "......\n";
LOCK_RELEASE(output_lck);
#endif
assembly->execute(nthread);

#if defined(CRIT_PERF_SCHED)
if(!(nthread-(assembly->leader))){
  t2 = std::chrono::system_clock::now();
  std::chrono::duration<double> elapsed_seconds = t2-t1;
  double ticks = elapsed_seconds.count();
  int index = 0;
  for(int k = 0; k<TABLEWIDTH; k++){
    if(wid[k] == assembly->width){
      index = k;  
    }
  }
  //Weight the newly recorded ticks to the old ticks 1:4 and save
  float oldticks = assembly->get_timetable( nthread,index);
  if(oldticks == 0){
    assembly->set_timetable(nthread,ticks,index);
  }
  else{
    assembly->set_timetable(nthread,((4*oldticks+ticks)/5),index);         
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
      continue;
    }
    LOCK_RELEASE(worker_lock[nthread]);

  // 3. try to steal 
  // TAO_WIDTH determines which threads participates in stealing
  // STEAL_ATTEMPTS determines number of steals before retrying the loop
    if(STEAL_ATTEMPTS && !(r_rand(&seed) % STEAL_ATTEMPTS)){
      int attempts = 1;
      do{
        do{
          random_core = (r_rand(&seed) % gotao_nthreads);
        } while(random_core == nthread  || random_core < 2);

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
#if TX2          
          if(nthread < pac0 && st->width == 4){
            st->width = 2;
          }
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

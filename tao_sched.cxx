/* assembly_sched.cxx -- integrated work stealing with assembly scheduling */

#include "tao.h"

#include <iostream>
#include <chrono>

#ifdef EXTRAE
#include "extrae_user_events.h"
#endif

// define the topology
int gotao_sys_topo[5] = TOPOLOGY;


// a PolyTask is either an assembly or a simple task
std::list<PolyTask *> worker_ready_q[MAXTHREADS];
#if defined(SUPERTASK_STEALING) || defined(TAO_STA)
GENERIC_LOCK(worker_lock[MAXTHREADS]);
//aligned_lock worker_lock[MAXTHREADS];
#endif

#ifdef DEBUG
GENERIC_LOCK(output_lck);
#endif

BARRIER *starting_barrier;

int gotao_nthreads;
int gotao_ncontexts;
int gotao_thread_base;
bool gotao_can_exit = false;

int worker_loop(int);

std::thread *t[MAXTHREADS];

// Initialize the gotao runtime 
int gotao_init_hw(int nthr, int thrb, int nhwc)
{
   if(nthr>=0)  gotao_nthreads = nthr;
   else { 	if(getenv("GOTAO_NTHREADS"))
			  gotao_nthreads = atoi(getenv("GOTAO_NTHREADS"));
		else      gotao_nthreads = GOTAO_NTHREADS;
	}

   if(nhwc>=0)  gotao_ncontexts = nhwc;
   else { 	if(getenv("GOTAO_HW_CONTEXTS"))
			  gotao_ncontexts = atoi(getenv("GOTAO_HW_CONTEXTS"));
		else      gotao_ncontexts = GOTAO_HW_CONTEXTS;
	}

   if(thrb>=0)  gotao_thread_base = thrb;
   else { 	if(getenv("GOTAO_THREAD_BASE"))
			  gotao_thread_base = atoi(getenv("GOTAO_THREAD_BASE"));
		else      gotao_thread_base = GOTAO_THREAD_BASE;
	}

   starting_barrier = new BARRIER(gotao_nthreads + 1);

   for(int i = 0; i < gotao_nthreads; i++)
      t[i]  = new std::thread(worker_loop, i);

#ifdef EXTRAE
   Extrae_init();
#endif
}

// Initialize gotao from environment vars or defaults
int gotao_init()
{
  return gotao_init_hw(-1, -1, -1);
}


int gotao_start()
{
  starting_barrier->wait();
}

int gotao_fini()
{
   gotao_can_exit = true;
   for(int i = 0; i < gotao_nthreads; i++)
     t[i]->join();
#ifdef EXTRAE
   Extrae_fini();
#endif
}

// push work into polytask queue
// if no particular queue is specified then try to determine which is the local
// queue and insert it there. This has some overhead, so in general the
// programmer should specify some queue
int gotao_push(PolyTask *pt, int queue)
{

#ifdef TAO_STA
  if((queue == -1) && (pt->affinity_queue != -1))
          queue = pt->affinity_queue;
  else 
#endif
     if (queue == -1)
          queue = sched_getcpu();

#if defined(SUPERTASK_STEALING) || defined(TAO_STA)
  LOCK_ACQUIRE(worker_lock[queue]);
#endif
  worker_ready_q[queue].push_front(pt);
#if defined(SUPERTASK_STEALING) || defined(TAO_STA)
  LOCK_RELEASE(worker_lock[queue]);
#endif // SUPERTASK_STEALING
#ifdef TIME_TRACE
  //Increment value of tasks in the system
  PolyTask::current_tasks.fetch_add(1);
#endif

}


// Push work when not yet running. This version does not require locks
// Semantics are slightly different here
// 1. the tid refers to the logical core, before adjusting with gotao_thread_base
// 2. if the queue is not specified, then put everything into the first queue
int gotao_push_init(PolyTask *pt, int queue)
{
#ifdef TAO_STA
  if((queue == -1) && (pt->affinity_queue != -1))
          queue = pt->affinity_queue;
  else 
#endif
  if(queue == -1) 
          queue = gotao_thread_base;

  worker_ready_q[queue].push_front(pt);
#ifdef TIME_TRACE
  //Increment value of tasks in the system
  PolyTask::current_tasks.fetch_add(1);
#endif
}



// alternative version that pushes to the back
int gotao_push_back_init(PolyTask *pt, int queue)
{
#ifdef TAO_STA
  if((queue == -1) && (pt->affinity_queue != -1))
          queue = pt->affinity_queue;
  else 
#endif
  if(queue == -1) 
          queue = gotao_thread_base;

  worker_ready_q[queue].push_back(pt);
#ifdef TIME_TRACE
  //Increment value of tasks in the system
  PolyTask::current_tasks.fetch_add(1);
#endif
}



#ifdef LOCK_FREE_QUEUE
LFQueue<PolyTask *> worker_assembly_q[MAXTHREADS];
GENERIC_LOCK(worker_assembly_lock[MAXTHREADS]);
//aligned_lock worker_assembly_lock[MAXTHREADS];
#else
std::list<PolyTask *> worker_assembly_q[MAXTHREADS]; // high prio queue for assemblies
std::mutex            worker_assembly_lock[MAXTHREADS];
#endif

// (approximate) number of steals (avoid using costly atomics here)
long int tao_total_steals = 0;

// http://stackoverflow.com/questions/3062746/special-simple-random-number-generator
long int r_rand(long int *s)
{
  *s = ((1140671485  * (*s) + 12820163) % (1<<24));
  return *s;
}


int worker_loop(int _nthread)
{
// new version without the contexts which anyway doesn't seem to work very well. 
// An alternative scheme for addressing hyperthreading to be developed later

// _nthread is the index we get via c++
// it identifies the virtual core in the XiTAO context (hence == nthread)
// to get the os thread we offset with the thread_base. This should just be used for the thread pinning
//            this thread is then called phys_core

#if 0
    int virtual_ncontexts = gotao_ncontexts / (MAXTHREADS/gotao_nthreads);	
    if(!virtual_ncontexts) virtual_ncontexts=1;

    int nthread = _nthread + gotao_thread_base;
    int hw_context_index = ((_nthread % virtual_ncontexts) * (MAXTHREADS/virtual_ncontexts));
    int core_index = _nthread / virtual_ncontexts;

    int phys_core = hw_context_index + core_index;
#elif 1
    int phys_core = gotao_thread_base + _nthread;
    int nthread = _nthread;

#endif
#ifdef DEBUG
    LOCK_ACQUIRE(output_lck);
    //std::cout << "gotao_ncontexts adjusted to " << virtual_ncontexts << std::endl;
    std::cout << "_nthread: " << _nthread << " mapped to physical core: "<< phys_core << std::endl;
    LOCK_RELEASE(output_lck);
#endif

    long int seed = 123456789;

    cpu_set_t cpu_mask;
    CPU_ZERO(&cpu_mask);
    CPU_SET(phys_core, &cpu_mask);

    sched_setaffinity(0, sizeof(cpu_set_t), &cpu_mask); 

    PolyTask *st = nullptr;
    int in = 0; // counter for to reduce computational intensiveness of the idle loop
    int idle_switch;

    if(getenv("GOTAO_IDLE_SWITCH"))
	idle_switch = atoi(getenv("GOTAO_IDLE_SWITCH"));
    else 
	idle_switch = IDLE_SWITCH;

    starting_barrier->wait();

    while(1){
        int random_core = 0;
        AssemblyTask *assembly = nullptr;
        SimpleTask *simple = nullptr;
#ifdef EXTRAE
        bool stealing = false;
#endif
        
        // 0. If a task is already provided via forwarding then exeucute it (simple task)
        //    or insert it into the assembly queues (assembly task)
        if(st){
            if(st->type == TASK_SIMPLE){
              SimpleTask *simple = (SimpleTask *) st;
#ifdef EXTRAE
              Extrae_event(EXTRAE_SIMPLE_START, st->taskid);
#endif
              simple->f(simple->args, nthread);
#ifdef EXTRAE
              Extrae_event(EXTRAE_SIMPLE_STOP, 0);
#endif
              st = simple->commit_and_wakeup(nthread);
              in = 0; // executed work, reset counter
              simple->cleanup();
              delete simple;
            }
            else if(st->type == TASK_ASSEMBLY)
            {
              AssemblyTask *assembly = (AssemblyTask *) st;
              int leader = ( _nthread / assembly->width) * assembly->width;
              assembly->leader = leader;
              int i;

#ifdef DEBUG
    LOCK_ACQUIRE(output_lck);
    //std::cout << "gotao_ncontexts adjusted to " << virtual_ncontexts << std::endl;
              std::cout << "Distributing assembly " << assembly->taskid << " with width " << assembly->width << " to workers [" << leader << "," << leader + assembly->width << ")" << std::endl;
    LOCK_RELEASE(output_lck);
#endif
  
              // take all locks in ascending order and insert assemblies
              // only once all locks are taken we unlock all queues
              // NOTE: locks are needed even in the case of LFQ, given that
              // out-of-order insertion can lead to deadlock.
              for(i = leader; i < leader + assembly->width; i++) {
#ifdef LOCK_FREE_QUEUE
                    LOCK_ACQUIRE(worker_assembly_lock[i]);
#else
                    worker_assembly_lock[i].lock();
#endif
                    worker_assembly_q[i].push_back(st);
              }
 
              // Locks are released from lower to upper as well
              // Order is not important for correctness, but anecdotal evidence shows that 
              //   performance is better in lower to upper order
              for(i = leader; i < leader + assembly->width; i++) {
#ifdef LOCK_FREE_QUEUE
                    LOCK_RELEASE(worker_assembly_lock[i]);
#else
                    worker_assembly_lock[i].unlock();
#endif
              }
    
              st = nullptr;
            }
            // proceed next iteration, since the inlined task may have
            // generated a forwarding
            // NOTE: take care with possible priority inversion here
            continue;
        }


        // 1. check for assemblies
#ifdef LOCK_FREE_QUEUE
        if(!worker_assembly_q[nthread].pop_front(&st))
                st = nullptr;
#else 
        {
        std::unique_lock<std::mutex> locker(worker_assembly_lock[nthread]);
        if(!worker_assembly_q[nthread].empty()){
           st = worker_assembly_q[nthread].front(); 
           worker_assembly_q[nthread].pop_front();
           }
        }
#endif


        // assemblies are inlined between two barriers
        if(st){
            int _final; // remaining
            assembly = (AssemblyTask *) st;

#ifdef EXTRAE
	    Extrae_event(EXTRAE_ASSEMBLY_START, assembly->taskid);
#endif
#ifdef SYNCHRONOUS_TAO
            assembly->barrier->wait();
#endif

//
//
//CHANGE TIME TRACE WAY HERE LATER
//
//

#ifdef INT_SOL
		 uint64_t t1,t2,freq,diff;
		 asm("isb; mrs %0, cntvct_el0" : "=r" (t1));
#endif
#ifdef TIME_TRACE
		
                std::chrono::time_point<std::chrono::system_clock> t1,t2;
		//If leader or assmebly width=1 start timing information
		if((!(nthread-(assembly->leader))) || (assembly->width == 1)){
                	t1 = std::chrono::system_clock::now();
		}
#endif
                        assembly->execute(nthread);
#ifdef INT_SOL
		  asm("isb; mrs %0, cntvct_el0" : "=r" (t2));
		  asm("isb; mrs %0, cntfrq_el0" : "=r" (freq));
		  diff = t2-t1;
		  assembly->set_timetable(nthread,diff,assembly->width);
		  //LOCK_ACQUIRE(output_lck);
		 std::cout << "Thread " << nthread << "completed with long " << t1 << " AND " << t2  << " and freq" << freq << std::endl;
		// LOCK_RELEASE(output_lck);
		  
			
#endif		  
#ifdef TIME_TRACE
	
		//If leader or assmebly width = 1 gather timing information
		 if((!(nthread-(assembly->leader))) || (assembly->width == 1)){
			//System clocks read if TIME_TRACE
                 	t2 = std::chrono::system_clock::now();
                 	std::chrono::duration<double> elapsed_seconds = t2-t1;
                 	double ticks = elapsed_seconds.count();
			//Index table based on width
		  	int tableind = (assembly->width == 4) ? (2) : ((assembly->width)-1); 
                  	assembly->set_timetable(nthread,ticks,tableind);         
		 }
#endif

#ifdef SYNCHRONOUS_TAO
            assembly->barrier->wait();
#endif
#ifdef EXTRAE
	    Extrae_event(EXTRAE_ASSEMBLY_STOP, 0);
#endif
            _final = (++assembly->threads_out_tao == assembly->width);

            st = nullptr;

            if(_final){ // the last exiting thread updates
#ifdef DEBUG
		LOCK_ACQUIRE(output_lck);
		std::cout << "Thread " << nthread << " completed assembly task " << assembly->taskid << std::endl;
		std::cout << "Current number of threads in the system" << PolyTask::current_tasks << std::endl;
		LOCK_RELEASE(output_lck);
#endif

               st = assembly->commit_and_wakeup(nthread);
               assembly->cleanup();
               delete assembly;
            }
            in = 0; // executed work, reset counter
           
            // proceed with next iteration to handle the forwarding case
            continue;
        }

        // 2. check own queue
        {
#if defined(SUPERTASK_STEALING) || defined(TAO_STA)
	  LOCK_ACQUIRE(worker_lock[nthread]);
#endif
          if(!worker_ready_q[nthread].empty()){
             st = worker_ready_q[nthread].front(); 
             worker_ready_q[nthread].pop_front();
#if defined(SUPERTASK_STEALING) || defined(TAO_STA)
	     LOCK_RELEASE(worker_lock[nthread]);
#endif
             continue;
             }
#if defined(SUPERTASK_STEALING) || defined(TAO_STA)
             LOCK_RELEASE(worker_lock[nthread]);
#endif
        }

        // 3. try to steal 
        // TAO_WIDTH determines which threads participates in stealing
        // STEAL_ATTEMPTS determines number of steals before retrying the loop
#if defined(SUPERTASK_STEALING)
        if(STEAL_ATTEMPTS) {
#ifdef EXTRAE
          if(!stealing){
            stealing = true;
            Extrae_event(EXTRAE_STEALING, 1);
            }
#endif
          int attempts = STEAL_ATTEMPTS; 
          do{
             do{
               random_core = (r_rand(&seed) % gotao_nthreads);
               } while(random_core == nthread);
            
             {
               LOCK_ACQUIRE(worker_lock[random_core]);
               if(!worker_ready_q[random_core].empty()){
                  st = worker_ready_q[random_core].back();  
                  worker_ready_q[random_core].pop_back();
                  tao_total_steals++;  // successful steals only
               }
               LOCK_RELEASE(worker_lock[random_core]);
             }

             
          }while(!st && (attempts-- > 0));
        if(st){
#ifdef EXTRAE
	     stealing = false;
	     Extrae_event(EXTRAE_STEALING, 0);
#endif
             continue;
	}
        } 

#endif

// measure of idleness (how long until the loop did not find work?)
	if(in < 10000)
        	in+=1;

// Based on the value of 'in' (idleness) we take some actions
// 'in' identifies the number of iterations during which no work has been found
// The higher this number, the less likely new work will be inserted, so we can start to 
// sleep increasingly longer intervals. However, there has to be an upper limit, set to 
// the time when the loop does no longer impact execution. There is no simple way to set this
// time, so we use a value determined experimentally.
//
// Attempt #1: if the architecture supportgs PAUSE then just reduce the speed
//

// again, this is mainly important for multithreading
// in multicore we can use it to slightly reduce the interference due to stealing
// but there is always a risk that the cores will not find a TAO in the AQ early enough
#if 0
#define IN_DIV 100
#ifdef PAUSE
	if(in > idle_switch) 
	        usleep(10*in/IN_DIV);
	else {
  	  for(int i = 0; i < in/IN_DIV; i++)
		_mm_pause();
	}
#else
	if(in > idle_switch)
	        usleep(in/IN_DIV);
#endif
#endif

        // 4. It may be that there are no more tasks in the flow
        //    this condition signals termination of the program

	// First check the number of actual tasks that have completed
        if(task_completions[nthread].tasks > 0){
                PolyTask::pending_tasks -= task_completions[nthread].tasks;
#ifdef DEBUG
		LOCK_ACQUIRE(output_lck);
		std::cout << "Thread " << nthread << " completed " << task_completions[nthread].tasks << " tasks. " 
 			"Pending tasks = " << PolyTask::pending_tasks << "\n";
		LOCK_RELEASE(output_lck);
#endif
                task_completions[nthread].tasks = 0;
        }

	// Next remove any virtual tasks from the per-thread task pool
	if(task_pool[nthread].tasks > 0){
		PolyTask::pending_tasks -= task_pool[nthread].tasks;
#ifdef DEBUG
		LOCK_ACQUIRE(output_lck);
		std::cout << "Thread " << nthread << " removed " << task_pool[nthread].tasks << " virtual tasks. " 
 			"Pending tasks = " << PolyTask::pending_tasks << "\n";
		LOCK_RELEASE(output_lck);
#endif
		task_pool[nthread].tasks = 0;
	}
        
	// Finally check if the program has terminated
        if(gotao_can_exit && (PolyTask::pending_tasks <= 0)) 
        {
                break;
        }

    }
    return 0;
}


// Below initializes the static component of the PolyTask class
//  = 0 cannot be specified since c++ atomics do not support static initialization
// On the other hand, static members must be initialized in global scope to avoid an "undefined reference" error
// Luckily the default constructor (initialize to '0') is exactly what I need 

std::atomic<int> PolyTask::pending_tasks;
struct completions task_completions[MAXTHREADS];
struct completions task_pool[MAXTHREADS];

#ifdef TIME_TRACE
std::atomic<int> PolyTask::current_tasks;
#endif

// need to declare the static class memeber
#if defined(DEBUG) || defined(EXTRAE)
std::atomic<int> PolyTask::created_tasks;
#endif

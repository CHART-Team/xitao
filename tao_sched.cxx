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
#if defined(SUPERTASK_STEALING) || defined(TAO_PLACES)
GENERIC_LOCK(worker_lock[MAXTHREADS]);
//aligned_lock worker_lock[MAXTHREADS];
#endif

#ifdef DEBUG
GENERIC_LOCK(output_lck);
#endif

BARRIER *starting_barrier;

int gotao_nthreads;
int gotao_thread_base;

int worker_loop(int);

std::thread *t[MAXTHREADS];


int gotao_init(int nthr, int thrb)
{
   if(nthr>=0)  gotao_nthreads = nthr;
   else         gotao_nthreads = GOTAO_NTHREADS;

   if(thrb>=0)  gotao_thread_base = thrb;
   else         gotao_thread_base = GOTAO_THREAD_BASE;

   starting_barrier = new BARRIER(gotao_nthreads + 1);
   for(int i = 0; i < gotao_nthreads; i++)
      t[i]  = new std::thread(worker_loop, i);

#ifdef EXTRAE
   Extrae_init();
#endif
}

int gotao_start()
{
  starting_barrier->wait();
}

int gotao_fini()
{
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

// if queue has a value, we take that
// however, we don't really want the user to do that, so should it be deprecated?
#ifdef TAO_PLACES
  // if no queue is specified, then the affinity has precedence
  if((queue == -1) && (pt->affinity_queue != -1))
          queue = pt->affinity_queue;
  else 
#endif
  // otherwise we go for local scheduling
     if (queue == -1)
          queue = sched_getcpu();

#if defined(SUPERTASK_STEALING) || defined(TAO_PLACES)
  LOCK_ACQUIRE(worker_lock[queue]);
#endif
  worker_ready_q[queue].push_front(pt);
#if defined(SUPERTASK_STEALING) || defined(TAO_PLACES)
  LOCK_RELEASE(worker_lock[queue]);
#endif // SUPERTASK_STEALING
}

// Push work when not yet running. This version does not require locks
// Semantics are slightly different here
// 1. the tid refers to the logical core, before adjusting with gotao_thread_base
// 2. if the queue is not specified, then put everything into the first queue
int gotao_push_init(PolyTask *pt, int queue)
{
#ifdef TAO_PLACES
  // if no queue is specified, then the affinity has precedence
  if((queue == -1) && (pt->affinity_queue != -1))
          queue = pt->affinity_queue;
  else 
#endif
  if(queue == -1) 
          queue = gotao_thread_base;

 // std::cout << "Pushing task into Queue: " << queue << std::endl;
  worker_ready_q[queue].push_front(pt);
}

// version that pushes in the back instead
int gotao_push_back_init(PolyTask *pt, int queue)
{
#ifdef TAO_PLACES
  // if no queue is specified, then the affinity has precedence
  if((queue == -1) && (pt->affinity_queue != -1))
          queue = pt->affinity_queue;
  else 
#endif
  if(queue == -1) 
          queue = gotao_thread_base;

 // std::cout << "Pushing task into Queue: " << queue << std::endl;
  worker_ready_q[queue].push_back(pt);
}
#ifdef LOCK_FREE_QUEUE
LFQueue<PolyTask *> worker_assembly_q[MAXTHREADS];
GENERIC_LOCK(worker_assembly_lock[MAXTHREADS]);
//aligned_lock worker_assembly_lock[MAXTHREADS];
#else
std::list<PolyTask *> worker_assembly_q[MAXTHREADS]; // high prio queue for assemblies
std::mutex             worker_assembly_lock[MAXTHREADS];
#endif

// approximate number of steals (avoid using costly atomics here)
long int tao_total_steals = 0;

// http://stackoverflow.com/questions/3062746/special-simple-random-number-generator
long int r_rand(long int *s)
{
  *s = ((1140671485  * (*s) + 12820163) % (1<<24));
  return *s;
}

int worker_loop(int _nthread)
{
    int phys_core = _nthread + gotao_thread_base;
    long int seed = 123456789;

    // affinity
    cpu_set_t cpu_mask;
    CPU_ZERO(&cpu_mask);
    CPU_SET(phys_core, &cpu_mask);
    
    sched_setaffinity(0, sizeof(cpu_set_t), &cpu_mask); 

    PolyTask *st = nullptr;

  //  wait so that all start at the same time
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
              simple->f(simple->args, phys_core);
#ifdef EXTRAE
	      Extrae_event(EXTRAE_SIMPLE_STOP, 0);
#endif
              st = simple->commit_and_wakeup(phys_core);
              simple->cleanup();
              delete simple;
            }
            else if(st->type == TASK_ASSEMBLY)
            {
              AssemblyTask *assembly = (AssemblyTask *) st;
#if (NUMA_ALIGNMENT > 0)
              int leader = ( phys_core / NUMA_ALIGNMENT) * NUMA_ALIGNMENT;
#else
              int leader = ( phys_core / assembly->width) * assembly->width;
#endif
              assembly->leader = leader;
              int i;
  
              // take all locks in ascending order and insert assemblies
              // only once all locks are taken we unlock all queues
              // NOTE: locks are needed even in the case of LFQ, given that
              // out-of-order insertion can lead to deadlock. Instead of
              // mutexes, could use spinlock to fully avoid OS stuff
              for(i = leader; i < leader + assembly->width; i++) {
#ifdef LOCK_FREE_QUEUE
		    LOCK_ACQUIRE(worker_assembly_lock[i]);
//                    while(worker_assembly_lock[i].lock.exchange(true)) {}              
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
       //             worker_assembly_lock[i].lock = false;
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
        if(!worker_assembly_q[phys_core].pop_front(&st))
                st = nullptr;
#else 
        {
        std::unique_lock<std::mutex> locker(worker_assembly_lock[phys_core]);
        if(!worker_assembly_q[phys_core].empty()){
           st = worker_assembly_q[phys_core].front(); 
           worker_assembly_q[phys_core].pop_front();
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
            assembly->execute(phys_core);
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
		std::cout << "Thread " << phys_core << " completed assembly task " << assembly->taskid << std::endl;
		LOCK_RELEASE(output_lck);
#endif 
               st = assembly->commit_and_wakeup(phys_core);
               assembly->cleanup();
               delete assembly;
            }
           
            // proceed with next iteration to handle the forwarding case
            continue;
        }

        // 2. check own queue
        {
#if defined(SUPERTASK_STEALING) || defined(TAO_PLACES)
	  LOCK_ACQUIRE(worker_lock[phys_core]);
       //   while(worker_lock[phys_core].lock.exchange(true)) {}              
#endif
          if(!worker_ready_q[phys_core].empty()){
             st = worker_ready_q[phys_core].front(); 
             worker_ready_q[phys_core].pop_front();
#if defined(SUPERTASK_STEALING) || defined(TAO_PLACES)
	     LOCK_RELEASE(worker_lock[phys_core]);
       //      worker_lock[phys_core].lock = false;
#endif
             continue;
             }
#if defined(SUPERTASK_STEALING) || defined(TAO_PLACES)
             LOCK_RELEASE(worker_lock[phys_core]);
             //worker_lock[phys_core].lock = false;
#endif
        }

        // 3. try to steal 
        // TAO_WIDTH determines which threads participates in stealing
        // STEAL_ATTEMPTS determines number of steals before retrying the loop
#if defined(SUPERTASK_STEALING)
        if((phys_core % TAO_WIDTH == 0) && STEAL_ATTEMPTS) {
#ifdef EXTRAE
	  if(!stealing){
		stealing = true;
		Extrae_event(EXTRAE_STEALING, 1);
		}
#endif
          int attempts = STEAL_ATTEMPTS; 
          do{
             do{
               random_core = gotao_thread_base + (r_rand(&seed) % gotao_nthreads);
               } while(random_core == phys_core);
            
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

        // 4. It may be that there are no more tasks in the flow
        //    this condition signals termination of the program

	// First check the number of actual tasks that have completed
        if(task_completions[phys_core].tasks > 0){
                PolyTask::pending_tasks -= task_completions[phys_core].tasks;
#ifdef DEBUG
		LOCK_ACQUIRE(output_lck);
		std::cout << "Thread " << phys_core << " completed " << task_completions[phys_core].tasks << " tasks. " 
 			"Pending tasks = " << PolyTask::pending_tasks << "\n";
		LOCK_RELEASE(output_lck);
#endif
                task_completions[phys_core].tasks = 0;
        }

	// Next remove any virtual tasks from the per-thread task pool
	if(task_pool[phys_core].tasks > 0){
		PolyTask::pending_tasks -= task_pool[phys_core].tasks;
#ifdef DEBUG
		LOCK_ACQUIRE(output_lck);
		std::cout << "Thread " << phys_core << " removed " << task_pool[phys_core].tasks << " virtual tasks. " 
 			"Pending tasks = " << PolyTask::pending_tasks << "\n";
		LOCK_RELEASE(output_lck);
#endif
		task_pool[phys_core].tasks = 0;
	}
        
	// Finally check if the program has terminated
        if(PolyTask::pending_tasks <= 0) 
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

// need to declare the static class memeber
#if defined(DEBUG) || defined(EXTRAE)
std::atomic<int> PolyTask::created_tasks;
#endif

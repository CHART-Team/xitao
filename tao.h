// tao.h - Task Assembly Operator

#ifndef _TAO_H
#define _TAO_H

#include <sched.h>
#include <unistd.h>
#include <thread>
#include <list>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <iostream>
#include <cmath>
#include "lfq-fifo.h"
#include "config.h"

extern int gotao_thread_base;
extern int gotao_nthreads;

extern int gotao_sys_topo[5];

#define GET_TOPO_WIDTH_FROM_LEVEL(x) gotao_sys_topo[x]

// a "sleeping" C++11 barrier for a set of threads
class cxx_barrier
{
public:
    cxx_barrier(unsigned int threads) : nthreads(threads), pending_threads(threads) {}

    bool wait ()
    {
        std::unique_lock<std::mutex> locker(barrier_lock);

        if(!pending_threads) 
            pending_threads=nthreads;

        --pending_threads;
        if (pending_threads > 0)
        {       
            threadBarrier.wait(locker);
        }
        else
        {
            threadBarrier.notify_all();
        }
    }

    // monitors
    std::mutex barrier_lock;
    std::condition_variable threadBarrier;

    int pending_threads;

    // Number of synchronized threads
    const unsigned int nthreads;
};

// spin barriers can be faster when fine-grained synchronization is desired
// They furthermore simplify the tracking of overhead

#ifdef PAUSE
 #include <immintrin.h>
#endif

// this is a spinning C++11 thread barrier for a single set of processors
// it is based on 
// http://stackoverflow.com/questions/8115267/writing-a-spinning-thread-barrier-using-c11-atomics
// plus several modifications
class spin_barrier
{
public:
    spin_barrier (unsigned int threads) 
               { 
                        nthreads = threads;
                        step_atmc = 0;
                        nwait_atmc = 0;
               }

    bool wait ()
    {
        unsigned int lstep = step_atmc.load ();

        if (nwait_atmc.fetch_add (1) == nthreads - 1)
        {
            /* OK, last thread to come.  */
            nwait_atmc.store (0, std::memory_order_release); 
            // reset waiting threads 
            // NOTE: release removes an "mfence", making it 10-20% faster depending on the case
            step_atmc.fetch_add (1);   // release the barrier 
            return true;
        }
        else // spin
        {
            while (step_atmc.load () == lstep)                     
#ifdef PAUSE
                // better not be too stressed while spinning
                _mm_pause()
#endif                        
                        ; 
            return false;
        }
    }

protected:
    /* Number of synchronized threads */
    unsigned int nthreads;

    /* Number of threads currently spinning */
    std::atomic<unsigned int> nwait_atmc;

    /* track steps. this is not to track progress but to implement spinning */
    std::atomic<int> step_atmc;

};

typedef void (*task)(void *, int);

#define TASK_SIMPLE   0x0
#define TASK_ASSEMBLY 0x1


class PolyTask;
struct aligned_lock {
         std::atomic<bool> lock __attribute__((aligned(64)));
};

#ifdef TTS
#warning "Using Test and Test and Set (TTS) implementation"
#define GENERIC_LOCK(l)  aligned_lock l;
#define LOCK_ACQUIRE(l)  while(l.lock.exchange(true)) {while(l.lock.load()){ }}
#define LOCK_RELEASE(l)  l.lock.store(false);
#else
#warning "Using Test and Set (TS) Implementation"
#define GENERIC_LOCK(l)  aligned_lock l;
#define LOCK_ACQUIRE(l)  while(l.lock.exchange(true)) {}
#define LOCK_RELEASE(l)  l.lock.store(false);

#endif

// a PolyTask is either an assembly or a simple task 
extern std::list<PolyTask *> worker_ready_q[MAXTHREADS];

#if defined(SUPERTASK_STEALING) || defined(TAO_STA)
extern aligned_lock worker_lock[MAXTHREADS];
#endif 


// ASSEMBLY QUEUES
#ifdef LOCK_FREE_QUEUE
extern LFQueue<PolyTask *> worker_assembly_q[MAXTHREADS]; 
extern aligned_lock worker_assembly_lock[MAXTHREADS];
#else
extern std::list<PolyTask *> worker_assembly_q[MAXTHREADS]; 
extern std::mutex             worker_assembly_lock[MAXTHREADS];
#endif

#ifdef DEBUG
extern GENERIC_LOCK(output_lck);
#endif

extern long int tao_total_steals;

struct completions{
        int tasks __attribute__((aligned(64)));
};

extern completions task_completions[MAXTHREADS];
extern completions task_pool[MAXTHREADS];

class PolyTask{
        public:
           PolyTask(int t, int _nthread=0) : type(t) 
           {
                    refcount = 0;
#ifdef TAO_STA
#define GOTAO_NO_AFFINITY (1.0)
                    affinity_relative_index = GOTAO_NO_AFFINITY;
                    affinity_queue = -1;
#endif
#if defined(DEBUG) || defined(EXTRAE)
                    taskid = created_tasks += 1;
#endif
                    if(task_pool[_nthread].tasks == 0){
                        pending_tasks += TASK_POOL;
                        task_pool[_nthread].tasks = TASK_POOL-1;
#ifdef DEBUG
                        std::cout << "Requested: " << TASK_POOL << " tasks. Pending is now: " << pending_tasks << "\n";
#endif
                        }
                    else task_pool[_nthread].tasks--;
                    threads_out_tao = 0;
#ifdef TIME_TRACE
	 	    criticality=0;
#endif
            }
	
#ifdef TIME_TRACE
	   //Ciritcality value to indicate critical path, higher value -> more critical
	   int criticality;
#endif

           int type;

#if defined(DEBUG) || defined(EXTRAE)
           int taskid;
           static std::atomic<int> created_tasks;
#endif
           static std::atomic<int> pending_tasks;
#ifdef TIME_TRACE
           static std::atomic<int> current_tasks;
           static std::atomic<int> prev_top_task;
#endif

           std::atomic<int> refcount;
           std::list <PolyTask *> out;
           std::atomic<int> threads_out_tao;
           int width;  // number of resources that this assembly uses
#ifdef TIME_TRACE
           virtual double get_timetable(int thread, int index) = 0;
#endif
#ifdef TAO_STA
           // PolyTasks can have affinity. Currently these are specified on a unidimensional vector
           // space [0,1) of type float
           float affinity_relative_index; // [0,1) are valid affinities, >=1.0 means no affinity
           int   affinity_queue;          // this is the particular queue. When cloning an affinity, we just copy this value
                                          // Internally, GOTAO works only with queues, not stas

           int sta_to_queue(float x){
                 if(x >= GOTAO_NO_AFFINITY) 
                         affinity_queue = -1;
                 else if (x < 0.0) return 1;  // error, should it be reported?
                 else affinity_queue = (int) (x*gotao_nthreads);
                 return 0; 
           }

           int set_sta(float x) {    
                 affinity_relative_index = x;  // whenever a sta is changed, it triggers a translation
                 return sta_to_queue(x);
           } 

           float get_sta() { return affinity_relative_index; }    // return sta value

           int clone_sta(PolyTask *pt) { 
                affinity_relative_index = pt->affinity_relative_index;    
                affinity_queue = pt->affinity_queue; // make sure to copy the exact queue
                return 0;
           }
#endif

           void make_edge(PolyTask *t)
           {
               out.push_back(t);
               t->refcount++;
           }

#ifdef TIME_TRACE
//Scheduling FUNCTIONs

            //Considers time table for new width, only resquedules among own "group" for new width
            int F(int _nthread, PolyTask *it){
		int new_width=1;
		double shortest_exec=5;

		//0=w1, 1=w1, 2=w4
		for(int i=0; i<3; i++){
			int temp = std::pow(2,i);
			//pow(2,i) gives width based of index
			//nthread%width gives distance from group
			//nthread-distance gives possible placement of X-width tao in group
			if(it->get_timetable((_nthread-(_nthread%(temp))),i)*temp<shortest_exec){
				shortest_exec = it->get_timetable(_nthread%(temp),i)*temp;
				new_width = temp;
			}
		}
		//Return index to queueu and change width
		it->width=new_width;	
		return (_nthread-(_nthread%new_width));	
	   }

	 //Change width according to system load
	  int F2(int _nthread, PolyTask *it){
		int children = out.size();
		int ndx = _nthread;
		if((current_tasks+children) < MAXTHREADS){
			//Ceiling division of integeres (x+y-1)/y 
			//Guards for 8 cores and 3 widths????
			//Consider mod 4 as in F
			int ce = (((current_tasks+children)+MAXTHREADS)-1)/(current_tasks+children);
			it->width=ce;
			
			
		}else{
			ndx = F(_nthread,it);

		}
		return(ndx);

     	  }
	
          //Recursive function of CATS -criticality calculation
	  int set_criticality(){
		if((criticality)==0){
			int max=0;
             		for(std::list<PolyTask *>::iterator edges = out.begin();
                	edges != out.end();
                	++edges){
				int new_max =((*edges)->set_criticality());
				max = ((new_max>max) ? (new_max) : (max));
			}


			criticality=++max;

		}
	
		return criticality;
	  }

	 //Criticality-aware Functio scheduling
	int F3(int _nthread, PolyTask * it){
		int prio = 0;
 		if((it->criticality) >= (prev_top_task.load()-1)){
			prev_top_task.store(it->criticality);
			prio=1;
		} 
		return prio;
	}	

	//For choosing "best" for prio
	int find_thread(int _nthread, PolyTask * it){
		int ndx = _nthread;
		double shortest_exec=5;
		for(int k=0; k<MAXTHREADS; k++){
			if((it->get_timetable(k,0))<shortest_exec){
				shortest_exec = (it->get_timetable(k,0));
				ndx=k;
			}
		
		}
		
		return ndx;
	
	}
#endif

           PolyTask * commit_and_wakeup(int _nthread)
           {
             PolyTask *ret = nullptr;

#ifdef TIME_TRACE
 	     //Increment value of tasks in the system
	     //SHOULD THIS BE DONE, since processor still busy, but idk
             current_tasks.fetch_sub(1);
	     
#endif
             for(std::list<PolyTask *>::iterator it = out.begin();
                it != out.end();
                ++it)
                {
                int refs = (*it)->refcount.fetch_sub(1);
                if(refs == 1){
#ifdef DEBUG
                        LOCK_ACQUIRE(output_lck);
                        std::cout << "Task " << (*it)->taskid << " became ready" << std::endl;
                        LOCK_RELEASE(output_lck);
#endif 

#ifdef TIME_TRACE
			int ndx = _nthread;
			int prio = F3(_nthread, (*it));
			if (prio == 1){
				ndx=find_thread(_nthread, (*it));	
			  }

#if defined(SUPERTASK_STEALING) || defined(TAO_STA)
                            LOCK_ACQUIRE(worker_lock[ndx]);
#endif
                            worker_ready_q[ndx].push_front(*it);
#if defined(SUPERTASK_STEALING) || defined(TAO_STA)
                            LOCK_RELEASE(worker_lock[ndx]);
#endif




#else

                        if(!ret
#ifdef TAO_STA
                           // check the case affinity_queue == -1
                                && (((*it)->affinity_queue == -1) || (((*it)->affinity_queue/(*it)->width) == (_nthread/(*it)->width)))
#endif
){
                          //CURRENT OVERRIDE OF POS§
//#ifdef TIME_TRACE
                          //F2(_nthread,(*it)); //Since forward locally, we don't need the resulting nthread for now only width change
//#endif
                           ret = *it; // forward locally only if affinity matches
                        }else{
                            // otherwise insert into affinity queue, or in local queue
#ifdef TAO_STA
                            int ndx = (*it)->affinity_queue;
                            if((ndx == -1) || (((*it)->affinity_queue/(*it)->width) == (_nthread/(*it)->width)))
                                 ndx = _nthread;
#else
			    int ndx = _nthread;
#endif

//#ifdef TIME_TRACE
                            //ndx = F2(_nthread,(*it)); //Change width
//#endif
                            // seems like we acquire and release the lock for each assembly. 
                            // This is suboptimal, but given that TAO_STA makes the allocation
                            // somewhat random it simpifies the implementation. In the case that
                            // TAO_STA is not defined, we could optimize it, but is it worth?
#if defined(SUPERTASK_STEALING) || defined(TAO_STA)
                            LOCK_ACQUIRE(worker_lock[ndx]);
#endif
                            worker_ready_q[ndx].push_front(*it);
#if defined(SUPERTASK_STEALING) || defined(TAO_STA)
                            LOCK_RELEASE(worker_lock[ndx]);
#endif



                         } 

#endif
                    }
#ifdef TIME_TRACE
		//increment the number of tasks in the system
		current_tasks.fetch_add(1);
#endif
              }

             task_completions[_nthread].tasks++;
             return ret;
           }
           
           virtual int cleanup() = 0;
};

//#define BARRIER cxx_barrier
#define BARRIER spin_barrier

// the base class for assemblies is very simple. It just provides base functionality for derived
// classes. The sleeping barrier is used by TAO to synchronize the start of assemblies
class AssemblyTask: public PolyTask{
        public:
                AssemblyTask(int w, int nthread=0) : PolyTask(TASK_ASSEMBLY, nthread), leader(-1) 
                {
                    width = w;
#ifdef NEED_BARRIER
                    barrier = new BARRIER(w);
#endif
                    //std::cout << "New assembly " << taskid << " of width " << w << std::endl;
                }

#ifdef NEED_BARRIER
                BARRIER *barrier;
#endif
                int leader;

                virtual int execute(int thread) = 0;
#ifdef TIME_TRACE
                virtual int set_timetable(int thread, double t, int index) = 0;
//                virtual double get_timetable(int thread, int index) = 0;
#endif
#ifdef INT_SOL
                virtual int set_timetable(int thread, uint64_t t) = 0;

#endif
                ~AssemblyTask(){
#ifdef NEED_BARRIER
                      delete barrier;
#endif
                }  
};

class SimpleTask: public PolyTask{
    public:
            SimpleTask(task fn, void *a, int nthread=0) : PolyTask(TASK_SIMPLE, nthread), args(a), f(fn) 
        { 
          width = 1; 
        }

            void *args;
            task f;

};


// API calls
#define goTAO_init gotao_init
int gotao_init_hw(int, int,int);
int gotao_init();

#define goTAO_start gotao_start
int gotao_start();

#define goTAO_fini gotao_fini
int gotao_fini();

#define goTAO_push gotao_push
int gotao_push(PolyTask *, int queue=-1);
int gotao_push_init(PolyTask *, int queue=-1);
int gotao_push_back_init(PolyTask *, int queue=-1);

//events
enum extrae_events{
        EXTRAE_SIMPLE_START,
        EXTRAE_SIMPLE_STOP,
        EXTRAE_ASSEMBLY_START,
        EXTRAE_ASSEMBLY_STOP,
        EXTRAE_STEALING,
};


#endif // _TAO_H

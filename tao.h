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

#ifdef TIME_TRACE
#define MAX_WIDTH_INDEX (int)((std::log2(GOTAO_NTHREADS))+1)
#endif


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
#ifdef F3
		    //Init criticality to 0
	 	    criticality=0;
#endif
            }
	
#ifdef F3
	   //Ciritcality value to indicate critical path, higher value -> more critical
	   int criticality;
#endif

           int type;

#if defined(DEBUG) || defined(EXTRAE)
           int taskid;
           static std::atomic<int> created_tasks;
#endif
           static std::atomic<int> pending_tasks;
#if defined(F2) || defined(F3) || defined(BIAS)
	   //Static atomic of system load for load-based molding
           static std::atomic<int> current_tasks;
#endif
#ifdef F3
	   //Static atomic of current most critical task for criticality-based scheduling
           static std::atomic<int> prev_top_task;
#endif
#ifdef BIAS
	  //Static atomic of current weight threshold for weight-based scheduling
	  static std::atomic<double> bias;
#endif	   

           std::atomic<int> refcount;
           std::list <PolyTask *> out;
           std::atomic<int> threads_out_tao;
           int width;  // number of resources that this assembly uses

#ifdef TIME_TRACE
	   //Virtual declaration of performance table get/set within tasks
           virtual double get_timetable(int thread, int index) = 0;
           virtual int set_timetable(int thread, double t, int index) = 0;
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

#if defined(F1)  || defined(F2) || defined(F3) || defined(BIAS) 
//Scheduling FUNCTIONs

            //History-based molding
             int history_mold(int _nthread, PolyTask *it){

		//Base case width = 1, want to fill table if empty
		int new_width=1; 
		//Set initial high value of shortest recorded
		double shortest_exec=100;
 
		//We are not interested in reszing to all cores so MAXWIDTH-1
		for(int i=0; i<(MAX_WIDTH_INDEX-1); i++){

			//pow(2,i) gives width based of index
			int width = std::pow(2,i);
			//Fetch the leaders past recorded value to compare with
			//Multiplication by width to account for the added resources
			double comp_perf = (width*(it->get_timetable((_nthread-(_nthread%(width))),i)));
			//If lower, update width
			if (comp_perf < shortest_exec){
				shortest_exec = comp_perf;
				new_width = width;
			} 

			//SAVE UNTIL TESTED ON PLATFORM
			/*
			if(temp*(it->get_timetable((_nthread-(_nthread%(temp))),i)) < shortest_exec){
				shortest_exec = (it->get_timetable((_nthread-(_nthread%(temp))),i))*temp;
				new_width = temp;
			}*/
		}  
		//Change width after results
		it->width=new_width;
		return _nthread; //0?	
	   } 
#endif

#if defined(F2) || defined(F3) || defined(BIAS)
	 //Load-based molding
	  int load_mold(int _nthread, PolyTask *it){
		int ndx = _nthread;
		//If the current systme load including the task to be woken up
		//does not exceed the resources in the system
		if((current_tasks+1) < MAXTHREADS){
			//Calculate ceiling devision of the number of resources
			//over the number of tasks to size accordingly
			int ce = (((current_tasks+1)+MAXTHREADS)-1)/(current_tasks+1);
			//Check if suggested width is of power of 2, not 0 and not maximum
			if(((ce &(ce-1)) == 0) && ce != 0 && ce != MAXTHREADS){
				it->width=ce;
			}
			//Should we have else case here where we set to 4 as we did before

			//SAVE UNTIL TESTED ON PLATFORM
			/*if((ce!=3)){
				if(ce!=8){
					it->width=ce;
				}
				else{
					it->width=4;
				}
			}else{
				it->width=4;
			}
			*/	
			
		//If the load was too high, consider history based molding		
		}else{
			ndx = history_mold(_nthread,it);

		}
		return(ndx); //0?

     	  } 
#endif
#ifdef F3	
          //Recursive function assigning criticality
	  int set_criticality(){
		//If the criticality is not yet set
		if((criticality)==0){
			int max=0;
			//For all children nodes
             		for(std::list<PolyTask *>::iterator edges = out.begin();
                	edges != out.end();
                	++edges){
				//Recursively call function, finding maximum of children
				int new_max =((*edges)->set_criticality());
				//If maximum is larger, update maximum
				max = ((new_max>max) ? (new_max) : (max));
			}

			//When all children have assigned crtiticality values
			//Set own criticality value to the maxiumum of children +1
			criticality=++max;

		}
	
		return criticality;
	  }

	//Determine if task is more or less critical then current system task
	int if_prio(int _nthread, PolyTask * it){
		int prio = 0;
		//If the criticality is larger or equal to current system load
 		if((it->criticality) >= (prev_top_task.load()-1)){
			//store own criticality as highest value 
			prev_top_task.store(it->criticality);
			//set return indication to task being prio
			prio=1;
		} 
		return prio;
	}	

	//Find suitable thread for prio task
	 int find_thread(int _nthread, PolyTask * it){
		//Inital thread is own
		int ndx = _nthread;
		//Set inital comparison value hihg
		double shortest_exec=100;
		double index = 0;
		//Compare threads with same width as task
		//Index corresponding to width is log2(width)
		index = log2(it->width);
		
		//Compare execution times of possible leaders for task
		//with current width, for loop interval is k=k+(wdith) 
		//to find next possible leader
		for(int k=0; k<MAXTHREADS; (k=k+(it->width))){
			double temp = (it->get_timetable(k,index));
			if(temp<shortest_exec){
				shortest_exec = temp;
				ndx=k;
			}
			//SAVE FOR TEST ON PLATFORM
			/*if((it->get_timetable(k,index))<shortest_exec){
				shortest_exec = (it->get_timetable(k,index));
				ndx=k;
			}*/
		
		}
		
		//Mold task to suitable width after suitable thread is found
		ndx = load_mold(ndx, it);
		
		return ndx;
	
	} 

	
#endif
#ifdef BIAS
	int weight_sched(int _nthread, PolyTask * it){
		int ndx=_nthread;
		double current_bias=bias;
		double div = 1;
		double little = 0; //Inital little value
		double big = 0; //Inital little value
		//Find index based on width
		double index = 0;
		index = log2(it->width);
		
		//Look att predefined little and big threads
		//with current width as index
	
		//ADD BIG LITTLE DEFINES
		little = it->get_timetable(0,index);
		big = it->get_timetable(4,index); 

		//If it has no recorded value on big, choose big
		if(!big){
			ndx=4;
		//If it has no recorded value on little, choose little
		}else if(!little){
			ndx=0;
		//Else, check if benifical to run on big or little
		}else{
			//If larger than current global value
			//Schedule on big cluster 
			//otherwise LITTLE
			div = little/big;
			if(div > current_bias){
				ndx=4;
			}else{
				ndx=0;
			}
			//Update the bias with a ratio of 1:6 
			//to the old bias
			current_bias=((6*current_bias)+div)/7;
			bias.store(current_bias);
		}
		
		//Choose a random big or little for scheduling
		ndx=((rand()%4)+ndx);
		//Mold the task to a new width
		load_mold(ndx,it);

		return ndx;
	}	

#endif

           PolyTask * commit_and_wakeup(int _nthread)
           {
             PolyTask *ret = nullptr;

#if defined(F2) || defined(F3) || defined(BIAS)
 	     //Decrement value of tasks in the system as we are committing
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

#if defined(F3) || defined(BIAS)
			int ndx2 = _nthread;
#ifdef F3
			int pr = if_prio(_nthread, (*it));
			if (pr == 1){
				ndx2=find_thread(_nthread, (*it));
				//ndx2=find_thread(ndx2, (*it));

				//ndx2=((rand()%4)+4); //CHOOSE A BIG CORE

			}else{
				//ndx2=((rand()%4)); //CHOOSE A LITTLE CORE
				ndx2=load_mold((rand()%8),(*it));


			}
#endif
#ifdef BIAS
			ndx2 = weight_sched(_nthread, (*it));
		
		/*	if(!ret &&  ndx2==((_nthread/4)*4)){// (((ndx2/4)*4)==((_nthread/4)*4))){
				ret= *it; //Forward locally, consider adding STA support

			}else{ */ 

#endif

#if defined(SUPERTASK_STEALING) || defined(TAO_STA)
                            LOCK_ACQUIRE(worker_lock[ndx2]);
#endif
                            worker_ready_q[ndx2].push_front(*it);
#if defined(SUPERTASK_STEALING) || defined(TAO_STA)
                            LOCK_RELEASE(worker_lock[ndx2]);
#endif

#ifdef BIAS
			//}
#endif




#else

                        if(!ret
#ifdef TAO_STA
                           // check the case affinity_queue == -1
                                && (((*it)->affinity_queue == -1) || (((*it)->affinity_queue/(*it)->width) == (_nthread/(*it)->width)))
#endif
){
                          //CURRENT OVERRIDE OF POS§
#ifdef F2
                          load_mold(_nthread,(*it)); //Since forward locally, we don't need the resulting nthread for now only width change
#endif

#ifdef F1
                           history_mold(_nthread,(*it)); 
#endif
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

#ifdef F2
                            ndx = load_mold(_nthread,(*it)); //Change width
#endif
#ifdef F1
                            history_mold(_nthread,(*it)); 
#endif
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

#if defined(F2) || defined(F3) || defined(BIAS)
		//increment the number of tasks in the system
		current_tasks.fetch_add(1);
#endif
                    }
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

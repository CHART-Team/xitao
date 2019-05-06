// add includes
// add fill cpu tao
// add gemm taos
//
//
// THATS ITTTTTTT  :D :D :Di
//
//
//
#include "tao.h"
#include "connected_layer.h"
#include <atomic>
#define PSLACK 1
using namespace std;





//CON_TAO1 fill CPU
class CON_TAO1 : public AssemblyTask{

        public:
        CON_TAO1(layer *_l, network *_net, int width) : l(_l),net(_net), AssemblyTask(width)
                {
                        blocksize = l->outputs / (width*PSLACK);
                        blocks = l->n / blocksize;
                        next=0;
                }
        int cleanup(){  }
        int execute(int threadid);
	//parameters for this TAO
	layer *l;
        network *net;
        atomic<int> next;
        int blocksize, blocks;
}; //end TAO1 class
// GEMM TAOS
class CON_TAO3 : public AssemblyTask{

        public:
#if defined(CRIT_PERF_SCHED)
	static float time_table[][GOTAO_NTHREADS];
#endif
        CON_TAO3(layer *_l, network *_net, int _len, int _index, int width) : l(_l),net(_net),len(_len), index(_index),  AssemblyTask(width)
                {
			next =0;
                }
        int cleanup(){  }
        int execute(int threadid);
        
	#if defined(CRIT_PERF_SCHED)
        int set_timetable(int threadid, float ticks, int index){
                  time_table[index][threadid] = ticks;
              }
              float get_timetable(int threadid, int index){
                  float time = 0;
                  time = time_table[index][threadid];
                  return time;
             }

        #endif
	
	//parameters for this TAO
        layer *l;
        network *net;
        atomic<int> next;
        int len, index;
};
class CON_TAO4 : public AssemblyTask{

        public:
#if defined(CRIT_PERF_SCHED)
	static float time_table[][GOTAO_NTHREADS];
#endif
        CON_TAO4(layer *_l, network *_net, int _len, int _index, int width) : l(_l),net(_net), len(_len), index(_index),  AssemblyTask(width)
                {
        		next = 0;     
                }
        int cleanup(){  }
        int execute(int threadid);
        
	#if defined(CRIT_PERF_SCHED)
        int set_timetable(int threadid, float ticks, int index){
                  time_table[index][threadid] = ticks;
              }
              float get_timetable(int threadid, int index){
                  float time = 0;
                  time = time_table[index][threadid];
                  return time;
             }

        #endif


	layer *l;
        network *net;
        atomic<int> next;
        int len, index;

        }; //end CON_TAO4 class

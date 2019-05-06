// add include:wq
// files.
#include "tao.h"
#include "convolutional_layer.h"
#include <atomic>

#define PSLACK 1
using namespace std;

//seriel code stubs
class seriel_TAO1 : public AssemblyTask{

        public:
#if defined(CRIT_PERF_SCHED)
	static float time_table[][GOTAO_NTHREADS];
#endif

        seriel_TAO1(layer *_l, network *_net, int width) : l(_l),net(_net), AssemblyTask(width)
                {
                }
        int cleanup(){  }
        int execute(int threadid);
        //parameters for this TAO
        layer *l;
        network *net;
}; //end seriel_TAO1 class

class seriel_TAO5 : public AssemblyTask{

        public:
#if defined(CRIT_PERF_SCHED)
	static float time_table[][GOTAO_NTHREADS];
#endif
        seriel_TAO5(layer *_l, network *_net, int width) : l(_l),net(_net), AssemblyTask(width)
                {       
                }
        int cleanup(){  }
        int execute(int threadid);
        //parameters for this TAO
        layer *l;
        network *net;
}; //end seriel_TAO5 class



// TAO 1

class TAO1 : public AssemblyTask{

	public:
 #if defined(CRIT_PERF_SCHED)
	static float time_table[][GOTAO_NTHREADS];
#endif
	TAO1(layer *_l, network *_net, int width) : l(_l),net(_net), AssemblyTask(width)
                {
			blocksize = l->n / (width*PSLACK);
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


// TAO 2

class TAO2 : public AssemblyTask{

        public:
#if defined(CRIT_PERF_SCHED)
	static float time_table[][GOTAO_NTHREADS];
#endif
        TAO2(layer *_l, network *_net, int width) : l(_l),net(_net), AssemblyTask(width)
                {
			blocksize = l->n / (width*PSLACK);
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
}; //end TAO2_1 class
/*class TAO2_2 : public AssemblyTask{

        public:
        TAO2_2(layer *_l, network *_net, int width) : l(_l),net(_net), AssemblyTask(width)
                {

                }
        int cleanup(){  }
        int execute(int threadid);
        //parameters for this TAO
        layer *l;
        network *net;
       
}; //end TAO2_2 class
*/
// TAO 3
//Gemm TAO 1
class TAO3 : public AssemblyTask{

        public:
#if defined(CRIT_PERF_SCHED)
	static float time_table[][GOTAO_NTHREADS];
#endif
        TAO3(layer *_l, network *_net, int _len, int _index,  int width) : l(_l),net(_net),len(_len), index(_index),  AssemblyTask(width)
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
}; //end TAO3 class
//GEMM TAO 2
class TAO4 : public AssemblyTask{

        public:
#if defined(CRIT_PERF_SCHED)
	static float time_table[][GOTAO_NTHREADS];
#endif
        TAO4(layer *_l, network *_net, int _len, int _index,  int width) : l(_l), net(_net), len(_len), index(_index),  AssemblyTask(width)
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
//        float *b;
	atomic<int> next;
        int  len, index;

        }; //end TAO4 class


// TAO 5
class TAO5 : public AssemblyTask{

        public:
        TAO5(layer *_l, network *_net, int width) : l(_l),net(_net), AssemblyTask(width)
                {

                }
        int cleanup(){  }
        int execute(int threadid);
        //parameters for this TAO
        layer *l;
        network *net;
     
}; //end TAO5 class

class TAO_barrier : public AssemblyTask{

        public:

#if defined(CRIT_PERF_SCHED)
        static float time_table[][GOTAO_NTHREADS];
#endif
        TAO_barrier(int width) : AssemblyTask(width)
                {
		calls = 0;
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
	atomic<int> calls;
        }; //end TAO_barrier class

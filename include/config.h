#ifndef _CONFIG_H
#define _CONFIG_H

#define GOTAO_THREAD_BASE 0
#define GOTAO_NO_AFFINITY (1.0)
#define STEAL_ATTEMPTS 100
#define TASK_POOL 100
#define TAO_STA 1
#define XITAO_MAXTHREADS 64
#define GOTAO_HW_CONTEXTS 1
#define DEBUG

namespace xitao {
  class config {
public: 
    static int thread_base;
    static int affinity;
    static int steal_attempts;
    static int task_pool;
    static int sta;
    static int hw_contexts;
    static bool enable_workstealing; 
    static bool use_performance_modeling;
    static int nthreads;
    static void printConfigs();
  };
}

#endif

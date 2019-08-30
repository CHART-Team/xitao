#ifndef _CONFIG_H
#define _CONFIG_H

//#define DEBUG

#define GOTAO_THREAD_BASE 0
#define GOTAO_NO_AFFINITY (1.0)
#define STEAL_ATTEMPTS 100
#define TASK_POOL 100
#define TAO_STA 1
#define XITAO_MAXTHREADS 8

#define L1_W   1
#define L2_W   2
#define L3_W   6
////#define L4_W   12
////#define L5_W   48
#define TOPOLOGY { L1_W, L2_W}
#define GOTAO_HW_CONTEXTS 1

//Defines for hetero environment if Weight or Crit_Hetero scheduling
//#define LITTLE_INDEX 0
//#define LITTLE_NTHREADS 4
//#define BIG_INDEX 4
//#define BIG_NTHREADS 4
#endif

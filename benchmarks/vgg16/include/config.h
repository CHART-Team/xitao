#ifndef _CONFIG_H
#define _CONFIG_H

//#define DEBUG
//
#define GOTAO_NTHREADS 16
#define GOTAO_THREAD_BASE 0
#define MAXTHREADS 20
////Defines for hetero environment if Weight or Crit_Hetero scheduling
////#define LITTLE_INDEX 0
////#define LITTLE_NTHREADS 4
////#define BIG_INDEX 4
////#define BIG_NTHREADS 4
//
// use a test-and-set lock
#define TTS
#define TAO_WIDTH 1
#define STEAL_ATTEMPTS 1
#define TASK_POOL 100
#define TAO_STA 1
#define IDLE_SWITCH 4
//
#define L1_W   1
#define L2_W   2
////#define L3_W   6
////#define L4_W   12
////#define L5_W   48
#define TOPOLOGY { L1_W, L2_W} //L3_W, L4_W, L5_W }
#define GOTAO_HW_CONTEXTS 1
#endif

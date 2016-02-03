/* loi.h 
 * Copyright 2014 Miquel Pericas and Tokyo Institute of Technology
 * This program is licensed under the terms of the New BSD License.
 * See LICENSE for full details
 *           
 * loi.h: Low Overhead Instrumetnation
 * This header provides access to a low-overhead profiler and access tracing library 
 */


#ifdef __cplusplus
extern "C" {
#endif

#pragma once
#define LOI

#include <sys/time.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>

// Some configurations require direct execution of syscalls. 
#include <sys/syscall.h>

// Some constants

// LOI_MAXTHREADS is the number of threads
// Use NUMTHREADS (loikrd.defs to tune the number of threads for the specific platform 
#define LOI_MAXTHREADS 1024

// Maximum Number of kernels under consideration 
// You can change this number if you know what you are doing
#define NUM_KERNELS  20    
#define NUM_PHASES   20
// 20 should be enough for most cases. I have not yet worked with more than 6 different kernels

// cycle counter - used to store TSC
typedef uint64_t ctimer_t;

// Structure for linking kernels and string identifiers
// Also provides information on phases
struct loi_kernel_info{
	int num_kernels;
	int num_phases;
	const char *kname[NUM_KERNELS]; // kernel names
	const char *phases[NUM_PHASES]; // phase names
	uint64_t    phase_kernels[NUM_PHASES];  // each phase can only have up to 64 kernels inside (1 bit per kernel)
};

// each entry is only touched by a single core and thread, so no locking is necessary
// structure is aligned to 64 bytes to avoid false sharing
struct loi_kernel_stats{
  ctimer_t cycles[NUM_KERNELS] __attribute__ ((aligned (64)));
  uint64_t num[NUM_KERNELS] __attribute__((packed)); 
};

// same for application phases
struct loi_phase_stats{
   ctimer_t exectime[NUM_PHASES] __attribute__ ((aligned (64)));
};

// kernel statistics (per thread) and full phase statistics
extern struct loi_kernel_stats kernelstats [LOI_MAXTHREADS] __attribute__((aligned(64)));
extern struct loi_phase_stats  phasestats __attribute__((aligned(64)));

/* Initialization */

// Measure the speed of the local TSC and setup buffer space for KRD tracing

int loi_init(void);

// TSC frequency is stored here
extern uint64_t TSCFREQ;

// some macros to convert from RDTSCP IDs into core ids and numa nodes
#define CORE(id)   (id & 0xfff)
#define NUMA(id)   ((id >> 12) & 0xff)

// Macro to access kernel timers and counters
//   Uses: Kernel #id, kernel stats pointer, thread index

#define   Ktime(kid,tid)  (kernelstats[tid].cycles[kid])
#define   Knum(kid,tid)   (kernelstats[tid].num[kid])

//#endif

// routines to access timers
// RDTSC and RDTSCP timer access

static inline ctimer_t __attribute__((unused)) rdtsc_clock_gettime(void)
{
      uint64_t clock_value;
      uint32_t low = 0;
      uint32_t high = 0;
      asm volatile ("rdtsc" : "=a" (low), "=d" (high));
      clock_value = ((uint64_t)high << 32) | (uint64_t)low;
      return (ctimer_t) clock_value;
}

// same functionality but using the serializing RDTSCP instruction.
// In practice it does not make much difference in terms of performance. 
// However, RDTSCP also provides a Core ID, so we introduce the second function below

// RDTSCP Cycle Timer
static inline ctimer_t __attribute__((unused)) rdtsc_clock_gettime_serial(void)
{
      uint64_t clock_value;
      uint32_t low = 0;
      uint32_t high = 0;
      uint32_t aux;
      asm volatile ("rdtscp" : "=a" (low), "=d" (high), "=c" (aux)); // we ignore the result in aux
      clock_value = ((uint64_t)high << 32) | (uint64_t)low;
      return (ctimer_t) clock_value;
}

// RDTSCP Cycle Timer and Core Id
static inline ctimer_t __attribute__((unused)) rdtsc_clock_gettime_serial_id(uint32_t *id)
{
      uint64_t clock_value;
      uint32_t low = 0;
      uint32_t high = 0;
      if (!id) return 0;
      asm volatile ("rdtscp" : "=a" (low), "=d" (high), "=c" (*id)); 
      clock_value = ((uint64_t)high << 32) | (uint64_t)low;
      return (ctimer_t) clock_value;
}


/* Cycle Timers */

// if not specified by user, use RDTSC timer by default
#ifndef TIMER
 #define TIMER 1
#endif

#if TIMER == 1
#define   loi_gettime(id)    rdtsc_clock_gettime()            // RDTSC method
#elif TIMER == 2
#define   loi_gettime(id)    rdtsc_clock_gettime_serial()     // RDTSCP method
#else 
  #error "Invalid timer specification"
#endif


/* Core Identification */ 

// if not specified by user use sched_getcpu() by default
#ifndef THREADID
#define THREADID 5
#endif

#if THREADID == 1
#warning "For GETTID core id method, use NUMTHREADS = LOI_MAXTHREADS"
#define threadid()  syscall(SYS_gettid)    // system call method, too much overhead
#elif THREADID == 2
#include <pthread.h>
#warning "For PTHREAD_SELF core id method, use NUMTHREADS = LOI_MAXTHREADS"
#define threadid() pthread_self()          // works but is outside POSIX
#elif THREADID == 3 
#warning "Sigle-thread core id method, multithreading activity is multiplexed onto one thread "
#define threadid() ((0))                   // this is just to avoid the API call when only 1 thread is used
#elif THREADID == 4
#define threadid() myth_get_worker_num()   // MassiveThreads method
#elif THREADID == 5
#ifdef _GNU_SOURCE
#include <sched.h>
#endif
#define threadid() sched_getcpu()   // Linux-specific, cross-architecture method
#else
#error "Invalid THREADID"
#endif

#if RDTSCP_ID
#if TIMER==1 
  #warning  "Using RDTSCP Core ID with RDTSC Timer. Are you sure RDTSCP is supported?"
#endif
#define  thread_index(coreid) (CORE(coreid))						  // coreid contains the NUMA and Core id 
#define  loi_gettime_id(a)    rdtsc_clock_gettime_serial_id(a)        // RDTSCP method
#else
#define  thread_index(coreid) ((threadid() %LOI_MAXTHREADS) & 0x3ff)     // use THREADID, and get rid of bits more than 1024
#define  loi_gettime_id(a)    loi_gettime(a)                         // use TIMER
#endif

// translate from cycle count to time
// loi_init() must have been called already for this to work
#define CTR(a) ((double) a/ (double) TSCFREQ)

// Generation of statistics
int loi_statistics(struct loi_kernel_info *, int total_threads); 

void loi_bench(void);

// Now include all the functionality for trace generation and processing
#include "krd.h"

void krd_bench();

// Here is an attempt to provide some concise macros for tracing your application
// The user needs to handle the scope by adding '{ }' around the pair of calls

// for OMP profiling
#define PRAGMA(x)                       _Pragma( #x )
 
#define phase_profile_start()           ctimer_t __pstart, __pstop; \
                                        __pstart = loi_gettime(); 

// for OpenMP parallel sections only the master should record phase profiles
#define phase_profile_start_omp()       ctimer_t __pstart, __pstop; \
                                        PRAGMA(omp master) \
                                        __pstart = loi_gettime();  \
                                        PRAGMA(omp barrier) 

#define phase_profile_stop(phase)       __pstop = loi_gettime(); \
                                        phasestats.exectime[phase] = __pstop - __pstart; 

#define phase_profile_stop_omp(phase)   PRAGMA(omp master) \
                                        { __pstop = loi_gettime(); \
                                        phasestats.exectime[phase] = __pstop - __pstart; }   \
                                        PRAGMA(omp barrier) 

#define kernel_profile_start()          uint32_t __coreid __attribute__((unused)); int __ndx; \
                                        ctimer_t __kstart, __kstop __attribute__((unused)); \
                                        __kstart = loi_gettime();

#define kernel_profile_stop(kindex)     __kstop = loi_gettime_id(&__coreid); \
                                        __ndx = thread_index(__coreid); \
                                        Ktime(kindex,__ndx) += __kstop - __kstart; \
                                        Knum(kindex,__ndx) = Knum(kindex,__ndx) + 1; 

#define phase_print_runtime(phase)      printf("Running time for phase %d is %g s\n", phase, CTR(phasestats.exectime[phase]));

// Here are some macros to simplify tracing
// They rely on the same variables as the profiler and should be used only together with profiling
// See examples for usage examples

#define  kernel_trace1(kid, id, s)      		TTrace_add_and_incr(__ndx, ((uint64_t) id),  __kstart,  s, kid)
#define  kernel_trace2(kid, id1, s1, id2, s2) 		TTrace_add_and_incr(__ndx, ((uint64_t) id1), __kstart, s1, kid); \
							TTrace_add_and_incr(__ndx, ((uint64_t) id2), __kstart, s2, kid)
#define  kernel_trace3(kid, id1, s1, id2, s2, id3, s3)	TTrace_add_and_incr(__ndx, ((uint64_t) id1), __kstart, s1, kid); \
							TTrace_add_and_incr(__ndx, ((uint64_t) id2), __kstart, s2, kid); \
							TTrace_add_and_incr(__ndx, ((uint64_t) id3), __kstart, s3, kid)

// Use these macros to specify whether the data access is read-only (KREAD) or ready-write (KWRITE)
#define KREAD(b)    			((int32_t)(b))
#define KWRITE(b)   			((-1)*((int32_t)(b)))

// 以上
//
#ifdef __cplusplus
}  // extern C
#endif

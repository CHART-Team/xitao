/* krd.h -- Kernel Reuse Distance
 * Copyright 2014 Miquel Pericas and Tokyo Institute of Technology
 * This program is licensed under the terms of the New BSD License.
 * See LICENSE for full details
 *
 * krd: data structures for keeping track of traces
 */

#ifndef LOI
 #error "Don't include <krd.h> directly, include <loi.h> instead"
#endif

void krd_start();

// default length of the trace to be collected (one thread only)
// The value can be overriden using krd_init_block() 
//
#define TRACE_LENGTH (1 << 26) 

// one trace entry
struct krd_id_tsc{         
    uint64_t id __attribute__ ((aligned(32)));       // address or block id, aligned to 32 bytes (2 elements per cache line in x86_64)
    uint64_t tsc;    // use full TSC counter (64 bits)
    int32_t size;    // bytes modified. The interpretation of this field depends on trace type:
                // RAW trace: >0 bytes read,         <0 bytes written
                // RWI trace: >0 bytes read/written, <0 bytes invalidated
// core/numa IDs are used to track the origin in merged traces. It is also used by the krd_tool to compute shared reuses/invalidations
    uint16_t core;  // core that generated this access
    int16_t kernel_id;  // kernel ID that is performing this access
//. uint16_t numa;  // numa node that generated this access
    };

// Magic numbers
// 2 trace formats are supported: 
//   RAW (Reads-and-Writes) traces
//   RWI (Read-Write-Invalidate) traces

#define RAW_TRACE_MAGIC   (0xfff)
#define RWI_TRACE_MAGIC   (0xffe)

// define access modes for RWI
#define ELEM_RDWR 0
#define ELEM_INV  1

// Structure used by app and tools to store trace elements
struct krd_thread_trace{
  uint64_t num_elems __attribute__((aligned(64)));  // total number of elements, alignment avoids false sharing
  uint64_t index     __attribute__((packed));       // auxiliary index used during merging and generating coherent traces
  struct krd_id_tsc * data_ids;                     // vector of elements
};

// the trace structure is global, and can be accesses from any position using just the thread index
// The following object points to the array of structures
//
extern struct krd_thread_trace * threadtraces __attribute__((aligned(64)));

// Initialization functions
int krd_init();  // NUMTHREADS workers, using default length of trace
int krd_init_block(int numthr, long len);  // specifying number of threads and how many elements to store per thread

// data models that are supported
enum data_models{
 ARRAY_PARTIAL,   // Array data. With partial overlaps
 ARRAY_DISJOINT,  // Array data. No partial overlaps (optimized with bloom filter)
 SPARSE           // Data is sparse, Partial overlaps cannot be tracked and thus should not occur
 };
// TODO: ARRAY_DISJOINT and SPARSE are basically the same. We may get rid of this in the future

// access granularity -- works only for ARRAY models
//enum granularity{
// WORDS,        // track accesses in 4-byte words
// CACHE_LINE    // track accesses in cache lines
//};


// Macros to manipulate the RAW/RWI traces
// Not very readable. Slowly removing them. Do not use at user level

#define  TTrace_elem(tid)       (threadtraces[tid].data_ids[threadtraces[tid].num_elems].id)
#define  TTrace_elem2(tid, ndx) (threadtraces[tid].data_ids[ndx].id)

#define  TTrace_size(tid)       (threadtraces[tid].data_ids[threadtraces[tid].num_elems].size)
#define  TTrace_size2(tid, ndx) (threadtraces[tid].data_ids[ndx].size)

#define  TTrace_core(tid)       (threadtraces[tid].data_ids[threadtraces[tid].num_elems].core)
#define  TTrace_core2(tid, ndx) (threadtraces[tid].data_ids[ndx].core)

#define  TTrace_id(tid)       (threadtraces[tid].data_ids[threadtraces[tid].num_elems].kernel_id)
#define  TTrace_id2(tid, ndx) (threadtraces[tid].data_ids[ndx].kernel_id)

#define  TTrace_numa(tid)       (threadtraces[tid].data_ids[threadtraces[tid].num_elems].numa)
#define  TTrace_numa2(tid, ndx) (threadtraces[tid].data_ids[ndx].numa)

#define  TTrace_tsc(tid)        (threadtraces[tid].data_ids[threadtraces[tid].num_elems].tsc)
#define  TTrace_tsc2(tid, ndx)  (threadtraces[tid].data_ids[ndx].tsc)
#define  TTrace_next_tsc(tid)   (threadtraces[tid].data_ids[threadtraces[tid].index].tsc)

#define  TTrace_incr_elems(tid)  (threadtraces[tid].num_elems++)
#define  TTrace_num_elems(tid)   (threadtraces[tid].num_elems)
#define  TTrace_incr_next(tid)  (threadtraces[tid].index++)
#define  TTrace_next(tid)       (threadtraces[tid].index)

// Add an entry to the trace
// Note that a thread can write at most TRACE_LENGTH entries. 
// Allocating larger entries using krd_init_block is mainly used for merging and coherence
static inline void TTrace_add_and_incr(uint32_t tid, uint64_t elem, uint64_t timestamp, int32_t size, int16_t kid)
{
     int coreid = tid; 
     if(threadtraces[coreid].num_elems < TRACE_LENGTH){
        threadtraces[coreid].data_ids[threadtraces[coreid].num_elems].tsc = timestamp;
        threadtraces[coreid].data_ids[threadtraces[coreid].num_elems].id = elem;
        threadtraces[coreid].data_ids[threadtraces[coreid].num_elems].size = size;
        threadtraces[coreid].data_ids[threadtraces[coreid].num_elems].kernel_id = kid; 
        threadtraces[coreid].num_elems++;
       }
} 

// modified version for benchmarking purposes
static inline void TTrace_add_and_incr_entry(uint32_t tid, uint64_t elem, uint64_t timestamp, int32_t size, int32_t entry, int16_t kid)
{
     int coreid = tid; 
     if(threadtraces[coreid].num_elems < TRACE_LENGTH){
    threadtraces[coreid].data_ids[entry].tsc = timestamp;
    threadtraces[coreid].data_ids[entry].id = elem;
    threadtraces[coreid].data_ids[entry].size = size;
    threadtraces[coreid].data_ids[entry].kernel_id = kid;
       }
} 

// save traces at program finalization
void krd_save_traces(void);

// benchmark the performance of tracing a single element (sequential and random access pattern)
void krd_bench(void);

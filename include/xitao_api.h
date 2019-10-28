/*! \file 
@brief Interfaces to the XiTAO Runtime.

This header define the interfaces of the runtime initialization, preperation 
of the TAODAG, finalization, etc.
*/
/** @example dotprod.cxx
 * An example parallization of 2 vectors dot product using XiTAO
 *  */
#ifndef _XITAO_API
#define _XITAO_API
#define goTAO_init gotao_init
#include "xitao.h"
#include <functional>
#include <mutex>
#include <atomic>
template <typename IterType>
class ParForTask;

class PolyTask;

//! Allocates/deallocates the XiTAO's runtime resources
/*!
  \param affinity_control Set the available resources for XiTAO
 */
int set_xitao_mask(cpu_set_t& user_affinity_setup);
//! Initialize the XiTAO Runtime
/*!
  \param nthr The number of XiTAO threads 
  \param thrb The logical thread id offset from the physical core mapping
  \param nhwc The number of hardware contexts
*/  
int gotao_init_hw(int nthr, int thrb, int nhwc);
//! Initialize the XiTAO Runtime using the environment variables XITAO_MAXTHREADS, GOTAO_THREAD_BASE and GOTAO_HW_CONTEXTS respectively
int gotao_init();
#define goTAO_start gotao_start
//! Triggers the start of the TAODAG execution
int gotao_start();
#define goTAO_fini gotao_fini
//! Finalize the runtime and makes sure that all workers have finished 
int gotao_fini();
#define goTAO_push gotao_push

/*! Push work into Polytask queue. if no particular queue is specified then try to determine which is the local. 
 queue and insert it there. This has some overhead, so in general the
 programmer should specify some queue
  \param pt The TAO to push 
  \param queue The queue to be pushed to (< XITAO_MAXTHREADS)
*/
int gotao_push(PolyTask *pt, int queue=-1);
//! Block master thread until DAG execution is finished, without having to finalize
void gotao_barrier();

//! Initialize the XiTAO Runtime
/*!
  \param iter_start is the iterator start
  \param end is the end of the loop
  \param func the spmd lambda function taking the iterator as an argument
*/ 
template <typename Proc, typename IterType>
ParForTask<IterType>* xitao_vec(IterType& iter_start, IterType const& end, Proc func) {
  ParForTask<IterType>* par_for = new ParForTask<IterType>(8, iter_start, end, func);
  return par_for;
}

template <typename Proc, typename IterType>
ParForTask<IterType>* xitao_vec_immediate(int parallelism, IterType& iter_start, IterType const& end, Proc func) {
  ParForTask<IterType>* par_for = new ParForTask<IterType>(parallelism, iter_start, end, func);
  gotao_push(par_for, 0);
  gotao_start();
  return par_for;
}

void __xitao_lock();
void __xitao_unlock();
//! wrapper for what constitutes and SPMD region
#define __xitao_vec_code(i, end, code) xitao_vec(i, end, [&](int& i){code;});
#define __xitao_vec_region(parallelism, i , end, code) xitao_vec_immediate(parallelism, i, end, [&](int& i, int& __xitao_thread_id){code;});
#endif

// push work into polytask queue
// if no particular queue is specified then try to determine which is the local
// queue and insert it there. This has some overhead, so in general the
// programmer should specify some queue
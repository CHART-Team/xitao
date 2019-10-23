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
template <typename Proc, typename IterType>
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
template <typename Proc, typename IterTyper>
ParForTask<Proc, IterTyper>* xitao_vec(IterTyper& iter_start, IterTyper const& end, Proc func) {
  ParForTask<Proc, IterTyper>* parfor = new ParForTask<Proc, IterTyper>(8, iter_start, end, func);
  return parfor;
}

template <typename Proc, typename IterTyper>
ParForTask<Proc, IterTyper>* xitao_vec_immediate(int parallelism, IterTyper& iter_start, IterTyper const& end, Proc func) {
  ParForTask<Proc, IterTyper>* parfor = new ParForTask<Proc, IterTyper>(parallelism, iter_start, end, func);
  gotao_push(parfor, 0);
  gotao_start();
  return parfor;
}
//! wrapper for what constitutes and SPMD region
#define __xitao_vec_code(i, end, code) xitao_vec(i, end, [&](int& i){code;});
#define __xitao_vec_region(parallelism,i , end, code) xitao_vec_immediate(parallelism, i, end, [&](int& i){code;});
#endif

// push work into polytask queue
// if no particular queue is specified then try to determine which is the local
// queue and insert it there. This has some overhead, so in general the
// programmer should specify some queue
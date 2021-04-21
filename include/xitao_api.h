/* BSD 3-Clause License

* Copyright (c) 2019-2021, contributors
* All rights reserved.

* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:

* 1. Redistributions of source code must retain the above copyright notice, this
*    list of conditions and the following disclaimer.

* 2. Redistributions in binary form must reproduce the above copyright notice,
*    this list of conditions and the following disclaimer in the documentation
*    and/or other materials provided with the distribution.

* 3. Neither the name of the copyright holder nor the names of its
*    contributors may be used to endorse or promote products derived from
*    this software without specific prior written permission.

* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

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
#include "xitao.h"
#include "tao.h"
#include <functional>
#include <mutex>
#include <atomic>
template <typename Proc, typename IterType>
class ParForTask;

class PolyTask;

//! Allocates/deallocates the XiTAO's runtime resources
/*!
  \param affinity_control Set the available resources for XiTAO
 */
void set_xitao_mask(cpu_set_t& user_affinity_setup);
//! Initialize the XiTAO Runtime
/*!
  \param nthr The number of XiTAO threads 
  \param thrb The logical thread id offset from the physical core mapping
  \param nhwc The number of hardware contexts
*/  
void xitao_init_hw(int nthr, int thrb, int nhwc);
//! Initialize the XiTAO Runtime using the environment variables XITAO_MAXTHREADS, GOTAO_THREAD_BASE and GOTAO_HW_CONTEXTS respectively
void xitao_init();

//! Initialize the XiTAO Runtime from comand line
void xitao_init(int argc, char** argv);

void xitao_start();
//! Finalize the runtime and makes sure that all workers have finished 
void xitao_fini();
//! Force the master to wait until the workers have processed all ready TAOs
void xitao_drain();

//! Sets the number of worker threads before the runtime is initialized
void xitao_set_num_threads(int nthreads);

/*! Push work into Polytask queue. if no particular queue is specified then try to determine which is the local. 
 queue and insert it there. This has some overhead, so in general the
 programmer should specify some queue
  \param pt The TAO to push 
  \param queue The queue to be pushed to (< XITAO_MAXTHREADS)
*/
void xitao_push(PolyTask *pt, int queue=-1);

//! Block master thread until DAG execution is finished, without having to finalize
void xitao_barrier();

//! Returns a handle to a ParForTask
/*!
  \param parallelism is the number of workers to execute the parallel for region
  \param iter_start is the iterator start
  \param end is the end of the loop
  \param func the spmd lambda function taking the iterator as an argument
  \param 0 for static scheduling, 1 for dynamic scheduling
*/ 
template <typename Proc, typename IterType>
ParForTask<Proc, IterType>* xitao_vec(int parallelism, IterType& iter_start, IterType const& end, Proc func, int sched_type) {
  ParForTask<Proc, IterType>* par_for = new ParForTask<Proc, IterType>(sched_type, iter_start, end, func, parallelism);
  return par_for;
}

//! Executes and returns a handle to a ParForTask
/*!
  \param width is the internal parallelism (width) of the underlying TAO
  \param iter_start is the iterator start
  \param end is the end of the loop
  \param func the spmd lambda function taking the iterator as an argument
  \param 0 for static scheduling, 1 for dynamic scheduling
*/ 
template <typename Proc, typename IterType>
ParForTask<Proc, IterType>* xitao_vec_immediate(int parallelism, IterType& iter_start, IterType const& end, Proc func, int sched_type) {
  ParForTask<Proc, IterType>* par_for = new ParForTask<Proc, IterType>(sched_type, iter_start, end, func, parallelism);
  xitao_push(par_for, 0);
  xitao_start();
  return par_for;
}

//! Executes and returns a handle to a ParForTask. However, subdivides the loop into finer grain tasks in order 
// to boost concurrency and adopt moldability 
/*!
  \param width is the internal parallelism (width) of the underlying TAOs
  \param iter_start is the iterator start
  \param end is the end of the loop
  \param func the spmd lambda function taking the iterator as an argument
  \param 0 for static scheduling, 1 for dynamic scheduling
  \param block_size number of data elements per internal task/tao
*/ 

template <typename Proc, typename IterType>
std::vector<ParForTask<Proc, IterType>* > xitao_vec_immediate_multiparallel(int width, IterType iter_start, IterType const& end, Proc func, int sched_type, int block_size) __attribute__((warn_unused_result));

template <typename Proc, typename IterType>
std::vector<ParForTask<Proc, IterType>* > xitao_vec_immediate_multiparallel(int width, IterType iter_start, IterType const& end, Proc func, int sched_type, int block_size) {     
  int nblocks = (end - iter_start + block_size - 1) / block_size;   
  std::vector<ParForTask<Proc, IterType>* > par_for;
  for(int i = 0; i < nblocks; ++i){
    IterType block_start = i * block_size; 
    IterType block_end   =(i < nblocks - 1)? block_start + block_size : end;
    par_for.push_back(new ParForTask<Proc, IterType>(sched_type, block_start, block_end, func, width));
    xitao_push(par_for[i], i % xitao_nthreads);
  }    
  xitao_start();
  return par_for;
}  

//! Executes and returns a handle to a ParForTask. However, subdivides the loop into finer grain tasks in order 
// to boost concurrency and adopt moldability 
/*!
  \param width is the internal parallelism (width) of the underlying TAOs
  \param iter_start is the iterator start
  \param end is the end of the loop
  \param func the spmd lambda function taking the iterator as an argument
  \param 0 for static scheduling, 1 for dynamic scheduling
  \param block_size number of data elements per internal task/tao
*/ 
template <typename Proc, typename IterType>
std::vector<ParForTask<Proc, IterType>* > xitao_vec_multiparallel(int width, IterType& iter_start, IterType const& end, Proc func, int sched_type, int block_size) {     
  int nblocks = (end - iter_start + block_size - 1) / block_size;   
  std::vector<ParForTask<Proc, IterType>* > par_for;
  for(int i = 0; i < nblocks; ++i){
    IterType block_start = i * block_size; 
    IterType block_end   =(i < nblocks - 1)? block_start + block_size : end;
    par_for.push_back(new ParForTask<Proc, IterType>(sched_type, block_start, block_end, func, width));
    // xitao_push(par_for[i], i % xitao_nthreads);
  }    
  return par_for;
}

//! Generic lock (not recommended, but added for correctness temporarily)
void __xitao_lock();

//! Generic unlock
void __xitao_unlock();
#endif

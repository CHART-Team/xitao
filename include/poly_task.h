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
@brief Defines the basic PolyTask type
*/

#ifndef _POLY_TASK_H
#define _POLY_TASK_H
#include <list>
#include <atomic>
#include "config.h"
#include "lfq-fifo.h"
#include "xitao_workspace.h"
#include "xitao_ptt.h"
// #include "queue_manager.h"
// #include "queue_manager.h"
//#include "xitao.h"

/*! the basic PolyTask type */
class PolyTask {
public:
  // PolyTasks can have affinity. Currently these are specified on a unidimensional vector
  // space [0,1) of type float. [0,1) are valid affinities, >=1.0 means no affinity
  float affinity_relative_index; 
  // this is the particular queue. When cloning an affinity, we just copy this value
  int   affinity_queue;          
  //Static atomic of current most critical task for criticality-based scheduling
  static std::atomic<int> prev_top_task;     
  //int criticality;
  int marker;
  // A pointer to the corresponding ptt table
  xitao::ptt_shared_type _ptt;  
  // An integer descriptor to distinguish the workload of several TAOs of the same type
  // it is mainly used by the scheduler when picking up the correct PTT
  size_t workload_hint;
  int type;
  // The leader task id in the resource partition
  int leader;
  int criticality;

#if defined(DEBUG)
  int taskid;
  static std::atomic<int> created_tasks;
#endif
  static std::atomic<int> pending_tasks;
  std::atomic<int> refcount;
  std::vector<PolyTask *> out;
  std::atomic<int> threads_out_tao;
  std::atomic<int> threads_in_tao;
  // the number of resources that this assembly uses
  int width; 
  // mold task to more than width = 1 
  bool mold; 

  //Virtual declaration of performance table get/set within tasks
  //! Virtual function that is called by the performance based scheduler to get an entry in PTT
  /*!
    \param threadid logical thread id that executes the TAO
    \param index the index of the width type 
    */
  virtual float get_timetable(int thread, int index);
  //! Virtual function that is called by the performance based scheduler to set an entry in PTT
  /*!
    \param threadid logical thread id that executes the TAO
    \param ticks the number of elapsed ticks
    \param index the index of the width type 
    */  
  virtual void set_timetable(int thread, float ticks, int index);
  //History-based molding
  // virtual void history_mold(int _nthread, PolyTask *it);
  //Recursive function assigning criticality
  // int set_criticality();
  // int set_marker(int i);
  //Determine if task is critical task
  int if_prio(int _nthread, PolyTask * it);
  // void globalsearch_PTT(int nthread, PolyTask * it);
  //Find suitable thread for prio task
  //int find_thread(int nthread, PolyTask * it);
  PolyTask(int t, int _nthread);
  
  //! Convert from an STA to an actual queue number
  /*!
    \param x a floating point value between [0, 1) that indicates the topology address in one dimension
  */
  void sta_to_queue(float x);
  //! give a TAO an STA address
  /*!
    \param x a floating point value between [0, 1) that indicates the topology address in one dimension
  */
  void set_sta(float x);
  //! get the current STA address of a TAO
  float get_sta();
  //! copy the STA of a TAO to the current TAO
  int clone_sta(PolyTask *pt);
  //! create a dependency to another TAO
  /*!
    \param t a TAO with which a happens-before order needs to be ensured (TAO t should execute after *this) 
  */
  void make_edge(PolyTask *t);
  //! complete the current TAO and wake up all dependent TAOs
  /*!
    \param _nthread id of the current thread
  */
  PolyTask * commit_and_wakeup(int _nthread);
  
  //! cleanup any dynamic memory that the TAO may have allocated
  virtual void cleanup() = 0;
};
#endif

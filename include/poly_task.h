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
//#include "xitao.h"

/*! the basic PolyTask type */
class PolyTask {
public:
  // PolyTasks can have affinity. Currently these are specified on a unidimensional vector
  // space [0,1) of type float. [0,1) are valid affinities, >=1.0 means no affinity
  float affinity_relative_index; 
  // this is the particular queue. When cloning an affinity, we just copy this value
  int   affinity_queue;          
#if defined(CRIT_PERF_SCHED)
  //Static atomic of current most critical task for criticality-based scheduling
  static std::atomic<int> prev_top_task;     
  //int criticality;
  int marker;
  // A pointer to the corresponding ptt table
  xitao::ptt_shared_type _ptt;  
  // An integer descriptor to distinguish the workload of several TAOs of the same type
  // it is mainly used by the scheduler when picking up the correct PTT
  size_t workload_hint;
#endif
  int type;
  // The leader task id in the resource partition
  int leader;
  int criticality;
  #if defined(DEBUG)
  int taskid;
  static std::atomic<int> created_tasks;
#endif
  static std::atomic<int> pending_tasks;

#ifdef WEIGHT_SCHED
  //Static atomic of current weight threshold for weight-based scheduling
  static std::atomic<double> bias;
#endif

  std::atomic<int> refcount;
  std::list <PolyTask *> out;
  std::atomic<int> threads_out_tao;
  int width; /*!< number of resources that this assembly uses */  

  //Virtual declaration of performance table get/set within tasks
#if defined(CRIT_PERF_SCHED)
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
  virtual void history_mold(int _nthread, PolyTask *it);
  //Recursive function assigning criticality
  int set_criticality();
  int set_marker(int i);
  //Determine if task is critical task
  int if_prio(int _nthread, PolyTask * it);
  int globalsearch_PTT(int nthread, PolyTask * it);
  static void print_ptt(float table[][XITAO_MAXTHREADS], const char* table_name);
  //Find suitable thread for prio task
  //int find_thread(int nthread, PolyTask * it);
#endif
  PolyTask(int t, int _nthread);
  
  //! Convert from an STA to an actual queue number
  /*!
    \param x a floating point value between [0, 1) that indicates the topology address in one dimension
  */
  int sta_to_queue(float x);
  //! give a TAO an STA address
  /*!
    \param x a floating point value between [0, 1) that indicates the topology address in one dimension
  */
  int set_sta(float x);
  //! get the current STA address of a TAO
  float get_sta();
  //! copy the STA of a TAO to the current TAO
  int clone_sta(PolyTask *pt);
  //! create a dependency to another TAO
  /*!
    \param t a TAO with which a happens-before order needs to be ensured (TAO t should execute after *this) 
  */
  void make_edge(PolyTask *t);
  
#ifdef WEIGHT_SCHED
  int weight_sched(int nthread, PolyTask * it);
#endif
  //! complete the current TAO and wake up all dependent TAOs
  /*!
    \param _nthread id of the current thread
  */
  PolyTask * commit_and_wakeup(int _nthread);
  
  //! cleanup any dynamic memory that the TAO may have allocated
  virtual void cleanup() = 0;
};
#endif

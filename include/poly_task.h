/*! \file 
@brief Defines the basic PolyTask type
*/

#ifndef _POLY_TASK_H
#define _POLY_TASK_H
#include <list>
#include <atomic>
#include "config.h"

//! A cache-aligned boolean type to implement locks
struct aligned_lock {
  std::atomic<bool> lock __attribute__((aligned(64)));
};

//! A cache-aligned integer to track the number of completed tasks by a master thread
struct completions{
  int tasks __attribute__((aligned(64)));
};
#define GENERIC_LOCK(l)  aligned_lock l;
#define LOCK_ACQUIRE(l)  while(l.lock.exchange(true)) {while(l.lock.load()){ }}
#define LOCK_RELEASE(l)  l.lock.store(false);

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
  int criticality;
  int marker;
#endif
  int type;

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
  int width;  // number of resources that this assembly uses

  //Virtual declaration of performance table get/set within tasks
#if defined(CRIT_PERF_SCHED)
  virtual float get_timetable(int thread, int index) = 0;
  virtual int set_timetable(int thread, float t, int index) = 0;
  //History-based molding
  int history_mold(int _nthread, PolyTask *it);
  //Recursive function assigning criticality
  int set_criticality();
  int set_marker(int i);
  //Determine if task is critical task
  int if_prio(int _nthread, PolyTask * it);
  int globalsearch_PTT(int nthread, PolyTask * it);
  //Find suitable thread for prio task
  int find_thread(int nthread, PolyTask * it);
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
  virtual int cleanup() = 0;
};
#endif

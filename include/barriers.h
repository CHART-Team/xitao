#ifndef _BARRIERS_H
#define _BARRIERS_H
#include <mutex>
#include <condition_variable>
#include <atomic>
// a "sleeping" C++11 barrier for a set of threads
class cxx_barrier
{
public:
  cxx_barrier(unsigned int threads);
  void wait ();
  // monitors
  std::mutex barrier_lock;
  std::condition_variable threadBarrier;
  int pending_threads;
  // Number of synchronized threads
  const unsigned int nthreads;
};

// spin barriers can be faster when fine-grained synchronization is desired
// They furthermore simplify the tracking of overhead

// this is a spinning C++11 thread barrier for a single set of processors
// it is based on 
// http://stackoverflow.com/questions/8115267/writing-a-spinning-thread-barrier-using-c11-atomics
// plus several modifications
class spin_barrier
{
public:
  spin_barrier (unsigned int threads);
  bool wait ();
protected:
    /* Number of synchronized threads */
  unsigned int nthreads;
    /* Number of threads currently spinning */
  std::atomic<unsigned int> nwait_atmc;
    /* track steps. this is not to track progress but to implement spinning */
  std::atomic<int> step_atmc;
};

#endif
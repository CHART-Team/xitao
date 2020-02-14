#include "barriers.h"
cxx_barrier::cxx_barrier(unsigned int threads) : nthreads(threads), pending_threads(threads) {}
void cxx_barrier::wait () {
  std::unique_lock<std::mutex> locker(barrier_lock);

  if(!pending_threads) 
    pending_threads=nthreads;

  --pending_threads;
  if (pending_threads > 0)
  {       
    threadBarrier.wait(locker);
  }
  else
  {
    threadBarrier.notify_all();
  }
}

// spin barriers can be faster when fine-grained synchronization is desired
// They furthermore simplify the tracking of overhead

// this is a spinning C++11 thread barrier for a single set of processors
// it is based on 
// http://stackoverflow.com/questions/8115267/writing-a-spinning-thread-barrier-using-c11-atomics
// plus several modifications
spin_barrier::spin_barrier (unsigned int threads) { 
  nthreads = threads;
  step_atmc = 0;
  nwait_atmc = 0;
}

bool spin_barrier::wait() {
  unsigned int lstep = step_atmc.load ();
  if (nwait_atmc.fetch_add (1) == nthreads - 1){
    /* OK, last thread to come.  */
    nwait_atmc.store (0, std::memory_order_release); 
    // reset waiting threads 
    // NOTE: release removes an "mfence", making it 10-20% faster depending on the case
    step_atmc.fetch_add (1);   // release the barrier 
    return true;
  }
  else{
    while (step_atmc.load () == lstep); 
    return false;
  }
}
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
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

#include "xitao_workspace.h"
namespace xitao {
  long int tao_total_steals = 0;  
  BARRIER* starting_barrier;
  cxx_barrier* tao_barrier;  
  struct completions task_completions[XITAO_MAXTHREADS];
  struct completions task_pool[XITAO_MAXTHREADS];
  int critical_path;
  int xitao_nthreads;
  int gotao_ncontexts;
  int gotao_thread_base;
  bool gotao_can_exit = false;
  bool gotao_initialized = false;
  bool gotao_started = false;
  bool resources_runtime_controlled = false;
  bool suppress_init_warnings = false;
  std::vector<int> runtime_resource_mapper;                                   // a logical to physical runtime resource mapper
  std::thread* t[XITAO_MAXTHREADS];
  std::vector<int> static_resource_mapper(XITAO_MAXTHREADS);
  std::vector<std::vector<int> > ptt_layout(XITAO_MAXTHREADS);
  std::vector<std::vector<std::pair<int, int> > > inclusive_partitions(XITAO_MAXTHREADS);
  GENERIC_LOCK(worker_lock[XITAO_MAXTHREADS]);
  GENERIC_LOCK(worker_assembly_lock[XITAO_MAXTHREADS]);
  std::mutex smpd_region_lock;
  std::mutex pending_tasks_mutex;
  std::condition_variable pending_tasks_cv;
#ifdef DEBUG
  GENERIC_LOCK(output_lck);
#endif
}

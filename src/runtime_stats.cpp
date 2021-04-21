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

#include "runtime_stats.h"
#include "xitao_ptt.h"
#include <iostream>
#include <fstream>
#include "config.h"
using namespace xitao;
using namespace std;

GENERIC_LOCK(stats_lock);
std::chrono::time_point<std::chrono::steady_clock> runtime_stats::thread_total_timer[XITAO_MAXTHREADS];
std::chrono::time_point<std::chrono::steady_clock> runtime_stats::thread_work_timer[XITAO_MAXTHREADS];
double runtime_stats::thread_work_time[XITAO_MAXTHREADS];
double runtime_stats::thread_total_time[XITAO_MAXTHREADS];
void runtime_stats::update_place_frequency(AssemblyTask* it) {
  if(config::use_performance_modeling) {
    int width  = it->width;
    int leader = it->leader;
    auto place = std::make_pair(leader, width);
    auto& table = it->_ptt->resource_place_freq;
    auto freq = table.find(place);
    LOCK_ACQUIRE(stats_lock);
    if(freq == table.end()) {
      table[place] = 1;
    } else {
      freq->second++;
    }
    LOCK_RELEASE(stats_lock);
  }
}

void runtime_stats::start_worker_stats(int nthread) {
  thread_total_timer[nthread] = std::chrono::steady_clock::now();
  thread_work_time[nthread] = 0.0;
}

void runtime_stats::start_worktime_epoch(int nthread) {
  thread_work_timer[nthread] = std::chrono::steady_clock::now();
}
void runtime_stats::end_worktime_epoch(int nthread) {
  std::chrono::duration<double> diff = std::chrono::steady_clock::now() - thread_work_timer[nthread];
  thread_work_time[nthread] += diff.count();
}
void runtime_stats::end_worker_stats(int nthread) {
  std::chrono::duration<double> diff = std::chrono::steady_clock::now() - thread_total_timer[nthread];
  thread_total_time[nthread] = diff.count();
}

void runtime_stats::dump_stats() {
  ofstream statsfile;
  statsfile.open (config::stats_file);
  if(config::use_performance_modeling) 
    for(auto&& ptt : xitao_ptt::runtime_ptt_tables) {
      auto&& type = ptt.first;
      auto&& freq = ptt.second->resource_place_freq;
      for(auto&& entry : freq) {
        statsfile << "Resource place stat" << "," << type.tao_type_index.name() << "," << type.workload_hint << 
        "," << entry.first.first << "," << entry.first.second << 
        "," << entry.second  << endl;
      }
    }

  for(int nthread = 0; nthread < xitao_nthreads; ++nthread) {
     statsfile << "Work time stats" << "," << nthread << "," << thread_work_time[nthread]  
      << "," << thread_total_time[nthread] - thread_work_time[nthread] << endl;
  }
  statsfile.close();
}
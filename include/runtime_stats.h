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

#ifndef RUNTIME_STATS
#define RUNTIME_STATS
#include "xitao_workspace.h"
#include "tao.h"
#include <vector>
#include <unordered_map>
#include <chrono>
namespace xitao {
  class runtime_stats {
  	static bool initialized;
  	static string stats_file_name;
  	static std::chrono::time_point<std::chrono::steady_clock> thread_total_timer[XITAO_MAXTHREADS];
  	static std::chrono::time_point<std::chrono::steady_clock> thread_work_timer[XITAO_MAXTHREADS];
  	static double thread_work_time[XITAO_MAXTHREADS];
  	static double thread_total_time[XITAO_MAXTHREADS];
public:
	static void update_place_frequency(AssemblyTask* it);
	static void dump_stats();
	static void start_worker_stats(int nthread);
	static void start_worktime_epoch(int nthread);
	static void end_worktime_epoch(int nthread);
	static void end_worker_stats(int nthread);
  };
}



#endif
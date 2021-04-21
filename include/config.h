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

#ifndef _CONFIG_H
#define _CONFIG_H

#define GOTAO_THREAD_BASE 0
#define GOTAO_NO_AFFINITY (1.0)
#define STEAL_ATTEMPTS 100
#define TASK_POOL 100
#define TAO_STA 1
#define XITAO_MAXTHREADS 64
#define GOTAO_HW_CONTEXTS 1
//#define DEBUG
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <getopt.h>
#include <string>
#include <iostream>
#include <iomanip>
using namespace std;
extern char *optarg;
extern int optind, opterr, optopt;
namespace xitao {
  class config {
public: 
    static int thread_base;
    static int affinity;
    static int steal_attempts;
    static int task_pool;
    static int sta;
    static int hw_contexts;
    static bool enable_workstealing; 
    static bool enable_local_workstealing; 
    static bool use_performance_modeling;
    static bool delete_executed_taos;
    static int nthreads;
    static string stats_file;
    static bool print_stats;
    static void print_configs();
    static void enable_stealing(int idle_tries_before_steal_count);
    static void disable_stealing();
    static void enable_performance_modeling();
    static void disable_performancem_modeling();
    static void enable_performance_modeling(bool min_par_cost, int table_refresh_rate, int old_tck_weight);
    static bool verbose;                          
    static const int stringLength = 25;           
    static const int decimal = 7;                 
    static const std::string xitao_args_prefix;

    static void init_config(int argc, char** argv, bool read_all_args=false);   
    static void usage(char* name); 
    template<typename T>
    static void formatted_print(std::string s, T v, bool fixed=true) {
    if (!verbose) return;
      std::cout << std::setw(stringLength) << std::left << s << " : ";
      if(fixed)
        std::cout << std::setprecision(decimal) << std::fixed;
      else
        std::cout << std::setprecision(1) << std::scientific;
      std::cout << v << std::endl;
    }
  };
}

#endif

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
#include <getopt.h>
#include <string>
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
    static void formatted_print(std::string s, T v, bool fixed=true);
  };
}

#endif

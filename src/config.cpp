#include "config.h"
#include "perf_model.h"
#include <thread>
#include <iostream>
using namespace xitao;
using namespace std;



bool perf_model::minimize_parallel_cost = true;
int  perf_model::refresh_frequency      = 10;
int  perf_model::old_tick_weight     = 4;

int  config::thread_base = GOTAO_THREAD_BASE;
int  config::affinity = GOTAO_NO_AFFINITY;
int  config::steal_attempts = STEAL_ATTEMPTS;
int  config::task_pool = TASK_POOL;
int  config::sta = TAO_STA;
int  config::hw_contexts = GOTAO_HW_CONTEXTS;
int  config::nthreads = std::thread::hardware_concurrency();
bool config::enable_workstealing = true; 
bool config::use_performance_modeling = true;


void config::enable_stealing(int idle_tries_before_steal_count = STEAL_ATTEMPTS) {
  config::enable_workstealing = true;
  config::steal_attempts = idle_tries_before_steal_count;
}
void config::disable_stealing() {
  config::enable_workstealing = false;
}
void config::enable_performance_modeling() {
  config::use_performance_modeling = true;
}
void config::disable_performancem_modeling() {
  config::use_performance_modeling = false;
}
void config::enable_performance_modeling(bool min_par_cost, int table_refresh_rate, int old_tck_weight){
  config::use_performance_modeling = true;
  perf_model::minimize_parallel_cost = min_par_cost;
  perf_model::refresh_frequency      = table_refresh_rate;
  perf_model::old_tick_weight     = old_tck_weight;
}

void config::print_configs() { 
  cout << "***************************************************" << endl;
  cout << "XiTAO is running with the following configurations:" << endl;
  cout << "***************************************************" << endl;
  cout << "Number of threads:" << config::nthreads << endl;
  cout << "Work stealing: " << ((config::enable_workstealing)? "enabled" : "disabled") << endl;         
  cout << "Performance model: " << ((config::use_performance_modeling)? "enabled" : "disabled") << endl;
  if(config::use_performance_modeling) {
    cout << "Performance model - Min parallel cost: " << ((perf_model::minimize_parallel_cost)? "enabled" : "disabled") << endl;
    cout << "Performance model - Refresh perf table entries every: " << perf_model::refresh_frequency << " tries" << endl;
    cout << "Performance model - Old tick weights: " << perf_model::old_tick_weight << "x the new ticks" << endl;
  }
  if(config::enable_workstealing) {
    cout << "Idle tries before sleep: " << config::steal_attempts << endl;
  }
  cout << "***************************************************" << endl;
}
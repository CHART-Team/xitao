#include "config.h"
#include <thread>
#include <iostream>
using namespace xitao;
using namespace std;

int  config::thread_base = GOTAO_THREAD_BASE;
int  config::affinity = GOTAO_NO_AFFINITY;
int  config::steal_attempts = STEAL_ATTEMPTS;
int  config::task_pool = TASK_POOL;
int  config::sta = TAO_STA;
int  config::hw_contexts = GOTAO_HW_CONTEXTS;
int  config::nthreads = std::thread::hardware_concurrency();
bool config::enable_workstealing = true; 
bool config::use_performance_modeling = false;

void config::printConfigs() { 
  cout << "XiTAO is running with the following configurations:" << endl;
  cout << "Number of threads:" << config::nthreads << endl;
  cout << "Work stealing: " << ((config::enable_workstealing)? "enabled" : "disabled") << endl;         
  cout << "Performance model: " << ((config::use_performance_modeling)? "enabled" : "disabled") << endl;
  cout << "Steal attempts: " << config::steal_attempts << endl;
}
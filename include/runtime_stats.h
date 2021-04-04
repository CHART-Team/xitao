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
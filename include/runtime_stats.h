#ifndef RUNTIME_STATS
#define RUNTIME_STATS
#include "xitao_workspace.h"
#include "tao.h"
#include <vector>
#include <unordered_map>
namespace xitao {
  class runtime_stats {
  	static bool initialized;
  	static string stats_file_name;
public:
	static void update_place_frequency(AssemblyTask* it);
	static void dump_stats();
  };
}



#endif
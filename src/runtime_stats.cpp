#include "runtime_stats.h"
#include "xitao_ptt.h"
#include <iostream>
#include <fstream>
#include "config.h"
using namespace xitao;
using namespace std;

GENERIC_LOCK(stats_lock);
void runtime_stats::update_place_frequency(AssemblyTask* it) {
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


void runtime_stats::dump_stats() {
  ofstream statsfile;
  statsfile.open (config::stats_file);
  for(auto&& ptt : xitao_ptt::runtime_ptt_tables) {
    auto&& type = ptt.first;
    auto&& freq = ptt.second->resource_place_freq;
    for(auto&& entry : freq) {
      statsfile << type.tao_type_index.name() << "," << type.workload_hint << 
      "," << entry.first.first << "," << entry.first.second << 
      "," << entry.second  << endl;
    }
  }
  statsfile.close();
}
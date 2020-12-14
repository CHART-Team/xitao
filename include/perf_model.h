#ifndef XITAO_PERF_MODEL
#define XITAO_PERF_MODEL
#include "xitao_workspace.h"
#include "poly_task.h"
#include <limits>
namespace xitao {

  class perf_model {

  public: 
    static bool minimize_parallel_cost; 
    static int refresh_frequency;
    static int old_tick_weight;
    
    static inline void global_search_ptt(PolyTask* it) {
      float shortest_exec = std::numeric_limits<float>::max();
      float comp_perf = 0.0f; 
      int new_width = 1; 
      int new_leader = -1;
      for(int leader = 0; leader < ptt_layout.size(); ++leader) {
        for(auto&& width : ptt_layout[leader]) {
          if(width <= 0) continue;
          auto&& ptt_val = it->get_timetable(leader, width);
          if(ptt_val == 0.0f) {
            new_width = width;
            new_leader = leader; 
            leader = ptt_layout.size(); 
            break;
          }
          if(minimize_parallel_cost) {
            comp_perf = width * ptt_val;
          } else {
            comp_perf = ptt_val;
          }
          if (comp_perf < shortest_exec) {
            shortest_exec = comp_perf;
            new_width = width;
            new_leader = leader;      
          }
        }
      }  
      it->width = new_width; 
      it->leader = new_leader;
    }

    static inline void history_mold_locally(int _nthread, PolyTask *it) {
      int new_width = 1; 
      int new_leader = -1;
      float shortest_exec = std::numeric_limits<float>::max();
      float comp_perf = 0.0f; 
      auto&& partitions = inclusive_partitions[_nthread];
      if(rand()%refresh_frequency != 0) { 
        for(auto&& elem : partitions) {
          int leader = elem.first;
          int width  = elem.second;
          auto&& ptt_val = it->get_timetable(leader, width);
          if(ptt_val == 0.0f) {
            new_width = width;
            new_leader = leader;       
            break;
          }
          if(minimize_parallel_cost) {
            comp_perf = width * ptt_val;
          } else {
            comp_perf = ptt_val;
          }
          if (comp_perf < shortest_exec) {
            shortest_exec = comp_perf;
            new_width = width;
            new_leader = leader;      
          }
        }
      } else { 
        auto&& rand_partition = partitions[rand() % partitions.size()];
        new_leader = rand_partition.first;
        new_width  = rand_partition.second;
      }
      if(new_leader != -1) {
        it->leader = new_leader;
        it->width  = new_width;
      }
    }


    static inline void update_performance_model(PolyTask* it, int thread, float ticks, int width) {
      float oldticks = it->get_timetable(thread, width);
      if(oldticks != 0) ticks = (old_tick_weight * oldticks + ticks) / (old_tick_weight + 1); 
      it->set_timetable(thread, ticks, width); 
    }

    static void set_performance_model_parameters(bool min_par_cost = false, int table_refresh_rate = 10, int old_tck_weight = 5) {
      minimize_parallel_cost = min_par_cost;
      refresh_frequency = table_refresh_rate; 
      old_tick_weight   = old_tck_weight;
    }

  };
 
}
#endif
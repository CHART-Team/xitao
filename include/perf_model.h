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
    static bool mold; 
    
    static inline void global_search_ptt(PolyTask* it) {
      float shortest_exec = std::numeric_limits<float>::max();
      float comp_perf = 0.0f; 
      int new_width = 1; 
      int new_leader = -1;
      int sz = ptt_layout.size(); 
      bool mold_task = mold && it->mold;                                 // check the global mold config and the task specific one
      for(int leader = 0; leader < sz; ++leader) {
        for(auto&& width : ptt_layout[leader]) {
          if(width <= 0 || (!mold && width > 1)) continue;
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
      auto& ptt = it->_ptt;
      if(ptt->cont_choices >= 10) { 
        it->leader = _nthread - _nthread % ptt->last_width;
        it->width  = ptt->last_width; 
        return;
      }
      int new_width = 1; 
      int new_leader = -1;
      float shortest_exec = std::numeric_limits<float>::max();
      float comp_perf = 0.0f; 
      auto&& partitions = inclusive_partitions[_nthread];
      bool mold_task = mold && it->mold;                                  // check the global mold config and the task specific one
      if(rand()%refresh_frequency != 0) { 
        for(auto&& elem : partitions) {
          int leader = elem.first;
          int width  = elem.second;
          if(!mold_task && width > 1) continue;
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

        if(ptt->last_width >= 0) { 
          if(ptt->last_width == new_width) ptt->cont_choices++;
          else ptt->cont_choices = 0;
        }
        ptt->last_width = new_width;
      } else if(mold_task) { 
        auto&& rand_partition = partitions[rand() % partitions.size()];
        new_leader = rand_partition.first;
        new_width  = rand_partition.second;
      } else {
        new_leader = _nthread;
        new_width  = 1;
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
  };
 
}
#endif

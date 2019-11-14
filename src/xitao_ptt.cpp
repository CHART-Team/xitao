#include "xitao_ptt.h"



tmap xitao_ptt::runtime_ptt_tables;

ptt_shared_type xitao_ptt::try_insert_table(PolyTask const& pt, size_t const& workload_hint) {
  // declare the tao_info to capture its type
  xitao_ptt_key tao_info (workload_hint, typeid(pt));

  // the PTT table 
  ptt_shared_type _ptt;

  // check if entry is new
  if(runtime_ptt_tables.find(tao_info) == runtime_ptt_tables.end()) {
    
    // allocate the ptt table and place it in shared pointer
    _ptt = std::make_shared<ptt_value_type>(XITAO_MAXTHREADS * XITAO_MAXTHREADS, 0);

    // insert the ptt table to the mapper
    runtime_ptt_tables.insert(std::make_pair(tao_info, _ptt));
  } else { 
    // get the existing value
    _ptt = runtime_ptt_tables[tao_info];
  } 
  // return the newly created or existing ptt table
  return _ptt;
}

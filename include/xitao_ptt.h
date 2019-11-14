#ifndef XITAO_PTT
#define XITAO_PTT
#include "config.h"
#include "xitao_workspace.h"
#include "poly_task.h"
#include <iostream>
using namespace xitao;

/*! basic tao_type_info that can be used as a key to PTT tables*/
//std::pair<std::type_index, size_t> tao_type_info;

/*! a class the manages the runtime ptt tables*/
class xitao_ptt {
public:
  /*! a static map that keeps track of the active PTT tables*/
  static tmap runtime_ptt_tables;  

public:
  //! try to insert a new PTT time table. 
  //! this is simply to make sure a single instance of the PTT is created per type
  /*!
    \param pt the polymorphic tao type 
    \param a hint to the runtime about the workload of the underlying TAO (if workload_a != workload_b, then workload_hint_a != workload_hine_b)
  */  
  static ptt_shared_type try_insert_table(PolyTask const& pt, size_t const& workload_hint);     
};



#endif
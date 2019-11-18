#ifndef XITAO_PTT
#define XITAO_PTT
#include "config.h"
#include "xitao_workspace.h"
#include "poly_task.h"
#include <iostream>
#include <algorithm>
using namespace xitao;

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
    \param workload_hint hint to the runtime about the workload of the underlying TAO (if workload_a != workload_b, then workload_hint_a != workload_hine_b)
  */  
  static ptt_shared_type try_insert_table(PolyTask* pt, size_t const& workload_hint);     

  //! release all pointers from the table side, and empties it. 
  //! note that the released pointers are smart pointers, and the TAOs that are created on the heap 
  //! have a handle to its corresponding pointer . TAO must be freed for the reference count to go to 0
  //! otherwise, a memory leak will potentially exist.   
  static void clear_tables();

  //! resets (zeros out) the PTT table that correspond to TAO type and workload
  /*!
    \param pt the polymorphic tao type 
    \param workload_hint hint to the runtime about the workload of the underlying TAO (if workload_a != workload_b, then workload_hint_a != workload_hine_b)
  */  
  static void reset_table(PolyTask* pt, size_t const& workload_hint);     
  

  //! prints the PTT table that correspond to TAO type and workload
  /*!
    \param table the table to be printed
    \param table_name a descriptive name for the table
  */  
  static void print_table(ptt_shared_type ptt, const char* table_name);

  template<typename tao_type>
  static void print_table(const char* table_name, size_t workload_hint = 0) {
     // declare the tao_info to capture its type
    xitao_ptt_key tao_info (workload_hint, typeid(tao_type));
    
    // the PTT table   
    ptt_shared_type _ptt;
    
    // check if entry is new  
    if(runtime_ptt_tables.find(tao_info) != runtime_ptt_tables.end()) {      
    
      // get the existing value
      _ptt = runtime_ptt_tables[tao_info];            
    } else {

      // the PTT does not exist
      std::cout <<"PTT does not exist for " << tao_info.tao_type_index.name() << std::endl;

      // exit the function
      return;
    } 
    print_table(_ptt, table_name);
  }
};



#endif
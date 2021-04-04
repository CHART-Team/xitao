#ifndef XITAO_PTT
#define XITAO_PTT
#include "config.h"
#include "xitao_workspace.h"
#include "poly_task.h"
#include <iostream>
#include <algorithm>
#include <map>
/*! a class that contains the performance model for task*/
class perf_data { 
public:
  xitao::ptt_value_type data;
  int last_leader;
  int last_width;
  int cont_choices;
  std::map<std::pair<int,int>,int> resource_place_freq; 
  perf_data(int table_size);
};

using namespace xitao;
/*! a class the manages the runtime ptt tables*/
class xitao_ptt {
public:
  /*! a static map that keeps track of the active PTT tables*/
  static tmap runtime_ptt_tables;  
  /*! the row size of the ptt table */
  static const size_t ptt_row_size;
  /*! widths start from 1. add 1 to col size to avoid subtracting 1 everytime width is indexed*/
  static const size_t ptt_col_size;
  /*! the full size of each ptt table. */
  static const size_t ptt_table_size;// = ptt_col_size * ptt_row_size; 
#if 0
  static const size_t elems_per_cache_line = config::cache_line_size / sizeof(ptt_value_type::value_type);

  static const size_t remainder_elements = (XITAO_MAXTHREADS + 1) % (elems_per_cache_line);
  /*! a pad size for thread entries in ptt to avoid false-sharing*/
  static const size_t pad_size =  (remainder_elements == 0)? 0 : elems_per_cache_line - remainder_elements;                                  
  /*! a row size to offset read values for thread entries*/
  static const size_t ptt_row_size =  (XITAO_MAXTHREADS + 1 + pad_size);
  /*! the size of the ptt table with padding. Add 1 to avoid substracting 1 every time width values are indexed*/
  static const size_t ptt_table_size = (XITAO_MAXTHREADS) * ptt_row_size;
#endif
  
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

  static void read_layout_table(const char* layout_file);
  static void prepare_default_layout(int nthreads);
  static void print_ptt_debug_info();
  static void clear_layout_partitions();

};



#endif

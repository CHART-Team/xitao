#include "xitao_ptt.h"
#include <fstream>
#include <cmath>
tmap xitao_ptt::runtime_ptt_tables;

ptt_shared_type xitao_ptt::try_insert_table(PolyTask* pt, size_t const& workload_hint) {
  // declare the tao_info to capture its type
  xitao_ptt_key tao_info (workload_hint, typeid(*pt));

  // the PTT table 
  ptt_shared_type _ptt;

  // check if entry is new
  if(runtime_ptt_tables.find(tao_info) == runtime_ptt_tables.end()) {
    
    // allocate the ptt table and place it in shared pointer
    _ptt = std::make_shared<ptt_value_type>(ptt_table_size, 0);

    // insert the ptt table to the mapper
    runtime_ptt_tables.insert(std::make_pair(tao_info, _ptt));
  } else { 
    // get the existing value
    _ptt = runtime_ptt_tables[tao_info];
  } 
  // return the newly created or existing ptt table
  return _ptt;
}

void xitao_ptt::clear_tables() {
  // iterate over all ptts in the hashmap to release the ownership of all PTT pointers

  for(auto&& ptt : runtime_ptt_tables) {

    // release the ownership of the ptt table 
    // remember that the TAO objects still have handles
    // and that they must be freed/deleted/scoped out for the smart pointer to be freed
    ptt.second.reset(); 
  }

  // clear the hash map
  runtime_ptt_tables.clear();
}

void xitao_ptt::reset_table(PolyTask* pt, size_t const& workload_hint){
 // declare the tao_info to capture its type
  xitao_ptt_key tao_info (workload_hint, typeid(*pt));
  // check if entry is new
  if(runtime_ptt_tables.find(tao_info) != runtime_ptt_tables.end()) {      
    // get the existing value
    ptt_shared_type _ptt = runtime_ptt_tables[tao_info];            
    // reset all PTT values
    std::fill(_ptt->begin(), _ptt->end(), 0);
  } 
}

void xitao_ptt::read_layout_table(const char* layout_file) {
  std::string line;      
  std::ifstream myfile(layout_file);
  int current_thread_id = -1; // exclude the first iteration
  if (myfile.is_open()) {
    bool init_affinity = false;
    while (std::getline(myfile,line)) {          
      size_t pos = 0;
      std::string token;
      if(current_thread_id >= XITAO_MAXTHREADS) {
        if(!suppress_init_warnings) 
          std::cout << "Fatal error: there are more partitions than XITAO_MAXTHREADS of: "\
           << XITAO_MAXTHREADS  << " in file: " << layout_file << std::endl;    
        exit(0);    
      }          
      int thread_count = 0;
      while ((pos = line.find(",")) != std::string::npos) {
        token = line.substr(0, pos);      
        int val = stoi(token);
        if(!init_affinity) static_resource_mapper[thread_count++] = val;  
        else { 
          if(current_thread_id + 1 >= xitao_nthreads) {
              if(!suppress_init_warnings) std::cout << "Fatal error: more configurations than \
                there are input threads in:" << layout_file << std::endl;    
              exit(0);
          }              
          ptt_layout[current_thread_id].push_back(val);
          if(val > 0) {
          for(int i = 0; i < val; ++i) {     
            if(current_thread_id + i >= XITAO_MAXTHREADS) {
              if(!suppress_init_warnings) std::cout << "Fatal error: illegal partition choices \
                for thread: " << current_thread_id <<" spanning id: " << current_thread_id + i \
                  << " while having XITAO_MAXTHREADS: " << XITAO_MAXTHREADS  << " in file: " << \
                layout_file << std::endl;    
              exit(0);           
            }
              inclusive_partitions[current_thread_id + i].push_back(std::make_pair(current_thread_id, val)); 
            }
          }              
        }            
        line.erase(0, pos + 1);
      }          
      if(line.size() > 0) {
        token = line.substr(0, line.size());      
        int val = stoi(token);
        if(!init_affinity) static_resource_mapper[thread_count++] = val;
        else { 
          ptt_layout[current_thread_id].push_back(val);
          for(int i = 0; i < val; ++i) {                
            if(current_thread_id + i >= XITAO_MAXTHREADS) {
              if(!suppress_init_warnings) std::cout << "Fatal error: illegal partition choices for \
                thread: " << current_thread_id <<" spanning id: " << current_thread_id + i << " while\
                 having XITAO_MAXTHREADS: " << XITAO_MAXTHREADS  << " in file: " << layout_file << std::endl;    
              exit(0);           
            }
            inclusive_partitions[current_thread_id + i].push_back(std::make_pair(current_thread_id, val)); 
          }              
        }            
      }
      if(!init_affinity) { 
        xitao_nthreads = thread_count; 
        init_affinity = true;
      }
      current_thread_id++;          
    }
    myfile.close();
  } else {
    if(!suppress_init_warnings) std::cout << "Fatal error: could not open hardware layout path " \
      << layout_file << std::endl;    
    exit(0);
  }
}

void xitao_ptt::prepare_default_layout(int nthreads) {
  std::vector<int> widths;             
  int count = nthreads;        
  std::vector<int> temp;        // hold the big divisors, so that the final list of widths is in sorted order 
  for(int i = 1; i < sqrt(nthreads); ++i){ 
    if(nthreads % i == 0) {
      widths.push_back(i);
      temp.push_back(nthreads / i); 
    } 
  }
  std::reverse(temp.begin(), temp.end());
  widths.insert(widths.end(), temp.begin(), temp.end());
  //std::reverse(widths.begin(), widths.end());        
  for(int i = 0; i < widths.size(); ++i) {
    for(int j = 0; j < nthreads; j+=widths[i]){
      ptt_layout[j].push_back(widths[i]);
    }
  }
  for(int i = 0; i < nthreads; ++i){
    for(auto&& width : ptt_layout[i]){
      for(int j = 0; j < width; ++j) {                
        inclusive_partitions[i + j].push_back(std::make_pair(i, width)); 
      }         
    }
  } 
}

void xitao_ptt::print_ptt_debug_info() {
#ifdef DEBUG
  for(int i = 0; i < static_resource_mapper.size(); ++i) { 
    std::cout << "[DEBUG] thread " << i << " is configured to be mapped to core id : " << static_resource_mapper[i] << std::endl;     
    std::cout << "[DEBUG] leader thread " << i << " has partition widths of : "; 
    for (int j = 0; j < ptt_layout[i].size(); ++j){
      std::cout << ptt_layout[i][j] << " ";      
    }
    std::cout << std::endl;
    std::cout << "[DEBUG] thread " << i << " is contained in these [leader,width] pairs : ";
    for (int j = 0; j < inclusive_partitions[i].size(); ++j){
      std::cout << "["<<inclusive_partitions[i][j].first << "," << inclusive_partitions[i][j].second << "]"; 
    }
    std::cout << std::endl;
  }
#endif
}

void xitao_ptt::clear_layout_partitions() {
  for(int i = 0; i < XITAO_MAXTHREADS; ++i) {
    inclusive_partitions[i].clear();
    ptt_layout[i].clear();
  }
}

void xitao_ptt::print_table(ptt_shared_type ptt, const char* table_name) { 
  // capture the underlying table
  auto&& table = *ptt;

  // print table details
  std::cout << std::endl << table_name <<  " PTT Stats: " << std::endl;
  for(int leader = 0; leader < ptt_layout.size() && leader < xitao_nthreads; ++leader) {
    auto row = ptt_layout[leader];
    std::sort(row.begin(), row.end());
    std::ostream time_output (std::cout.rdbuf());
    std::ostream scalability_output (std::cout.rdbuf());
    std::ostream width_output (std::cout.rdbuf());
    std::ostream empty_output (std::cout.rdbuf());
    time_output  << std::scientific << std::setprecision(3);
    scalability_output << std::setprecision(3);    
    empty_output << std::left << std::setw(5);
    std::cout << "Thread = " << leader << std::endl;    
    std::cout << "==================================" << std::endl;
    std::cout << " | " << std::setw(5) << "Width" << " | " << std::setw(9) << std::left << "Time" << " | " << "Scalability" << std::endl;
    std::cout << "==================================" << std::endl;
    for (int i = 0; i < row.size(); ++i) {
      int curr_width = row[i];
      if(curr_width <= 0) continue;
      auto comp_perf = table[(curr_width) * ptt_row_size + leader];
      std::cout << " | ";
      width_output << std::left << std::setw(5) << curr_width;
      std::cout << " | ";      
      time_output << comp_perf; 
      std::cout << " | ";
      if(i == 0) {        
        empty_output << " - ";
      } else if(comp_perf != 0.0f) {
        auto scaling = table[(row[0]) * ptt_row_size + leader]/comp_perf;
        auto efficiency = scaling / curr_width;
        if(efficiency  < 0.6 || efficiency > 1.3) {
          scalability_output << "(" <<table[(row[0]) * ptt_row_size + leader]/comp_perf << ")";  
        } else {
          scalability_output << table[(row[0]) * ptt_row_size + leader]/comp_perf;
        }
      }
      std::cout << std::endl;  
    }
    std::cout << std::endl;
  }
}
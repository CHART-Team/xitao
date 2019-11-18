#ifndef XITAO_PTT_KEY
#define XITAO_PTT_KEY
#include <typeindex>
#include <typeinfo>

struct xitao_ptt_key {
  // a hint to the runtime about the workload of the underlying TAO (if workload_a != workload_b, then workload_hint_a != workload_hine_b)
  std::size_t workload_hint;

  // the runtime type index of the TAOs
  std::type_index tao_type_index;      

public :
  // public constructor
  xitao_ptt_key(std::size_t _workload, std::type_index _type_index);

  // an equality operator for enabling hashmap lookup for ptt tables 
  bool operator == (const xitao_ptt_key &other) const;  
  
};

class xitao_ptt_hash { 

public: 
   // hash function for xitao_ptt_key
  std::size_t operator ()(const xitao_ptt_key &p) const;
}; 

//! A boost like function to combine the hash values
template <class T>
inline void hash_combine(std::size_t& seed, const T& v);

#endif
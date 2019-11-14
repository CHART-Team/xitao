#include "xitao_ptt_key.h"
#include <functional>
xitao_ptt_key::xitao_ptt_key (std::size_t _workload, std::type_index _index): 
                              workload_hint(_workload), tao_type_index(_index) { }

bool xitao_ptt_key::operator == (const xitao_ptt_key &other) const { 
  return (workload_hint == other.workload_hint
          && tao_type_index == other.tao_type_index);
}

// hash function for xitao_ptt_key
std::size_t xitao_ptt_hash::operator () (const xitao_ptt_key &p) const {
  auto seed = std::hash<std::size_t>()(p.workload_hint);
  hash_combine(seed, p.tao_type_index);
  return seed;
}

//! A boost like function to combine the hash values 
// (taken from stackoverflow https://stackoverflow.com/questions/2590677/how-do-i-combine-hash-values-in-c0x)
template <class T>
inline void hash_combine(std::size_t& seed, const T& v) {
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
}
#ifndef XITAO_MACROS
#define XITAO_MACROS
#include "xitao_api.h"
#include <type_traits>
//! wrapper for what constitutes and SPMD region returns a handle to a ParForTask for later insertion in a DAG
#define __xitao_vec_code(width, i, end, sched, code) xitao_vec(							\
															width, i, end,				\
															[&](int& i, int& __xitao_thread_id){\
																code;							\
															},									\
															sched);

//! wrapper for what constitutes and SPMD region. Immdediately executes a ParForTask
#define __xitao_vec_region(width, iter , end, sched, code) xitao_vec_immediate(											\
															width, iter, end,											\
															[&](int& loop_begin, int& loop_end, int& __xitao_thread_id){\
																for(int iter = loop_begin; iter < loop_end; ++iter) {   \
																	code;							   					\
																}														\
															},									   						\
															sched);

//! wrapper for what constitutes and SPMD region executed by concurrent tasks. 
#define __xitao_vec_multiparallel_region(width, iter , end, sched, block_size, code) xitao_vec_immediate_multiparallel(\
															width, iter, end,											\
															[&](int& loop_begin, int& loop_end, int& __xitao_thread_id){\
																for(int iter = loop_begin; iter < loop_end; ++iter) {   \
																	code;							   					\
																}														\
															},									   						\
															sched, block_size);

//! wrapper for what constitutes and SPMD region executed by concurrent tasks. 
#define __xitao_vec_multiparallel_region_sync(width, iter, begin, end, sched, block_size, code) xitao_vec_immediate_multiparallel(\
															width, begin, end,											\
															[&](int& loop_begin, int& loop_end, int& __xitao_thread_id){\
																for(int iter = loop_begin; iter < loop_end; ++iter) {   \
																	code;							   					\
																}														\
															},									   						\
															sched, block_size);											\
															gotao_drain();

//! wrapper for what constitutes and SPMD region executed by concurrent tasks. 
#define __xitao_vec_non_immediate_multiparallel_region(width, iter , end, sched, block_size, code) xitao_vec_multiparallel(\
															width, iter, end,											\
															[&](int& loop_begin, int& loop_end, int& __xitao_thread_id){\
																for(int iter = loop_begin; iter < loop_end; ++iter) {   \
																	code;							   					\
																}														\
															},									   						\
															sched, block_size);

//! wrapper for printing the PTT results on the data parallel region
#if CRIT_PERF_SCHED
#define __xitao_print_ptt(ref) 								std::remove_reference<decltype(ref)>::type::print_ptt( \
															std::remove_reference<decltype(ref)>::type::time_table,\
															"Vec Region PTT");
#else 
#define __xitao_print_ptt(ref) 						
#endif		
#endif
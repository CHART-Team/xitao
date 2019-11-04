#ifndef XITAO_MACROS
#define XITAO_MACROS
#include "xitao_api.h"
//! wrapper for what constitutes and SPMD region returns a handle to a ParForTask for later insertion in a DAG
#define __xitao_vec_code(parallelism, i, end, sched, code) xitao_vec(							\
															parallelism, i, end,				\
															[&](int& i, int& __xitao_thread_id){\
																code;							\
															},									\
															sched);

//! wrapper for what constitutes and SPMD region. Immdediately executes a ParForTask
#define __xitao_vec_region(parallelism, i , end, sched, code) xitao_vec_immediate(				\
															parallelism, i, end,				\
															[&](int& i, int& __xitao_thread_id){\
																code;							\
															},									\
															sched);
#endif
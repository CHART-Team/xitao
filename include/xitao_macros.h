/* BSD 3-Clause License

* Copyright (c) 2019-2021, contributors
* All rights reserved.

* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:

* 1. Redistributions of source code must retain the above copyright notice, this
*    list of conditions and the following disclaimer.

* 2. Redistributions in binary form must reproduce the above copyright notice,
*    this list of conditions and the following disclaimer in the documentation
*    and/or other materials provided with the distribution.

* 3. Neither the name of the copyright holder nor the names of its
*    contributors may be used to endorse or promote products derived from
*    this software without specific prior written permission.

* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

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
															xitao_drain();

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
#define __xitao_print_ptt(ref) 								std::remove_reference<decltype(ref)>::type::print_ptt( \
															std::remove_reference<decltype(ref)>::type::time_table,\
															"Vec Region PTT");
#endif

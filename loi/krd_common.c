/* krd_common.c 
 * Copyright 2014 Miquel Pericas and Tokyo Institute of Technology
 * This program is licensed under the terms of the New BSD License.
 * See LICENSE for full details
 *
 * krd_common.c: 
 * common data and functionality shared by both tools and tracing facility
 */

#include "loi.h"
#include "krd_common.h"

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>
#include <sys/mman.h>
#include <limits.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <stdlib.h>


// kernel traces
// each trace consists of a vector of elements and an index indicating the next element to be accessed/inserted
struct krd_thread_trace * threadtraces  __attribute__((aligned(64)));

// every tool works on only one of two modes: RAW or RWI
// This variable stores the current mode
//
int krd_trace_magic = -1; // -1 means not yet settled

// Allocate tracing structure with default parameters
// This function is not currently used. Preferred method is krd_init_block() -> see loi_init()
//
int krd_init()
{
  int i;

  // First allocate the basic structure, one for each thread
  threadtraces = (struct krd_thread_trace * ) calloc(LOI_MAXTHREADS, sizeof(struct krd_thread_trace));
  if(!threadtraces){
	printf("Error allocating traces array\n");
	exit(1);
  }
 
  // unknown number of threads -> use LOI_MAXTHREADS
  for(i = 0; i < LOI_MAXTHREADS; i++){
	threadtraces[i].index = 0;  // used by the interleaving tool
	threadtraces[i].num_elems = 0;
	threadtraces[i].data_ids = (struct krd_id_tsc *) mmap(NULL, sizeof(struct krd_id_tsc) * TRACE_LENGTH,  PROT_READ |  PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, 0, 0);
	if(!threadtraces[i].data_ids){
		printf("Error allocating trace\n");
		exit(1);
		}
	// most of the memory space allocated here will not really be touched, hence MAP_NORESERVE. 
	// It is unlikely that we run into memory problems. If we do we will get SIGSEGV (and deserve so) 
	}

  return 0;
}

// Allocate tracing structure with user-provided parameters
//
int krd_init_block(int numthreads, long tracelength)
{
  int i;
  // If not yet done, allocate the basic threading structure
  if(!threadtraces) 
   	 threadtraces = (struct krd_thread_trace * ) calloc(LOI_MAXTHREADS, sizeof(struct krd_thread_trace));
 
  if(!threadtraces){
	printf("Error allocating traces array\n");
	exit(1);
  }

  for(i = 0; i < numthreads; i++){
	threadtraces[i].index = 0; 
	threadtraces[i].num_elems = 0;
	threadtraces[i].data_ids = (struct krd_id_tsc *) mmap(NULL, sizeof(struct krd_id_tsc) * tracelength,  PROT_READ |  PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, 0, 0);
	if(!threadtraces[i].data_ids){
		printf("Error allocating trace\n");
		exit(1);
		}
	}

  return 0;
}

// Allocate the data structure for a particular thread entry 
// This is used by the merge/coherence tools to allocte the output buffer
int krd_init_index(int threadindex, long tracelength)
{
// Allocate tracing structure
  if (!threadtraces) 
	threadtraces = (struct krd_thread_trace * ) calloc(LOI_MAXTHREADS, sizeof(struct krd_thread_trace));
 
  if(((threadindex < 0) || (threadindex >= LOI_MAXTHREADS) || (tracelength < 0))) {
	printf("Bad threadindex/tracelength number %d/%ld at %s %d\n", threadindex, tracelength, __FILE__, __LINE__);
	return -1;
	}

  threadtraces[threadindex].index = 0;  
  threadtraces[threadindex].num_elems = 0;
  threadtraces[threadindex].data_ids = (struct krd_id_tsc *) mmap(NULL, sizeof(struct krd_id_tsc) * tracelength,  PROT_READ |  PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, 0, 0);
  if(!threadtraces[threadindex].data_ids){
	printf("Error allocating trace\n");
	exit(1);
  }

  return 0;
}


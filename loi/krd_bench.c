/* krd_bench.c
 * Copyright (c) 2014 Miquel Pericas and Tokyo Institute of Technology
 * This program is licensed under the terms of the New BSD License.
 * See LICENSE for full details
 * 
 * krd_bench.c: 
 *   benchmarking codes used to measure performance of the profiling
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

// benchmark the time for tracing two elements
void krd_bench_range_rand(long int range)
{
  ctimer_t tstart, tstop;
  uint32_t coreid __attribute__((unused)) = 0, ndx;
  tstart = loi_gettime_id(&coreid);
  int i, bench_iterations = 1e5;
  long entry;

  // Benchmark recording of trace and then reinitialize
  for(i = 0; i < bench_iterations; i++){
    loi_gettime();
    // do nothing
    loi_gettime_id(&coreid);
    ndx = thread_index(coreid);
    entry = random() % range;

    TTrace_add_and_incr_entry(ndx, (uint32_t) coreid, tstart, 1, entry, 0);
//    TTrace_add_and_incr_entry(ndx, (uint32_t) coreid, tstart, 1, entry+1);
//    TTrace_add_and_incr_entry(ndx, (uint32_t) coreid, tstart, 1, entry+2);
//    TTrace_add_and_incr_entry(ndx, (uint32_t) coreid, tstart, 1, entry+3);
//    TTrace_add_and_incr_entry(ndx, (uint32_t) coreid, tstart, 1, entry+4);
//    TTrace_add_and_incr_entry(ndx, (uint32_t) coreid, tstart, 1, entry+5);
//    TTrace_add_and_incr_entry(ndx, (uint32_t) coreid, tstart, 1, entry+6);
//    TTrace_add_and_incr_entry(ndx, (uint32_t) coreid, tstart, 1, entry+7);
    }

  tstop = loi_gettime();
  printf("Average time for tracing 1 element in random range %ld is %g nsec\n", range, 1e9*CTR((tstop-tstart)/bench_iterations));

  // reset counters
  TTrace_num_elems(ndx) = 0;
  TTrace_next(ndx) = 0;
}

// benchmark the time for tracing two elements
void krd_bench_range_seq(long int range)
{
  ctimer_t tstart, tstop;
  uint32_t coreid __attribute__((unused)) = 0, ndx = 0;
  tstart = loi_gettime_id(&coreid);
  int i, bench_iterations = range;

  // Benchmark recording of trace and then reinitialize
  for(i = 0; i < range; i++){
    loi_gettime();
    // do nothing
    loi_gettime_id(&coreid);
    ndx = thread_index(coreid);

    TTrace_add_and_incr_entry(ndx, (uint32_t) coreid, tstart, 1, i, 0);
    }

  tstop = loi_gettime();
  printf("Average time for tracing 1 element in sequential range %ld is %g nsec\n", range, 1e9*CTR((tstop-tstart)/bench_iterations));

  // reset counters
  TTrace_num_elems(ndx) = 0;
  TTrace_next(ndx) = 0;
}

// benchmark several sizes
void krd_bench()
{
	long int ranges[] = {100, 1000, 10000, 100000, 1000000, 10000000};
	unsigned int i;
	printf("The size of one entry in trace array is %lu bytes\n", sizeof(struct krd_id_tsc));
	for (i = 0; i < (sizeof(ranges)/sizeof(long int)); i++)
		krd_bench_range_rand(ranges[i]);
	for (i = 0; i < (sizeof(ranges)/sizeof(long int)); i++)
		krd_bench_range_seq(ranges[i]);
}


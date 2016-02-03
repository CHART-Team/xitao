/* loi.c 
 * Copyright 2014 Miquel Pericas and Tokyo Institute of Technology
 * This program is licensed under the terms of the New BSD License.
 * See LICENSE for full details
 *
 * Low Overhead Instrumentation 
 */ 

#include "loi.h"
#include <stdio.h>
#include <math.h>

// declaration of the kernel_stats structure
// I choose to declare it statically so that one indirection is avoided in each access
// Furthermore, for efficiency it is aligned to the cache line boundary
struct loi_kernel_stats kernelstats [LOI_MAXTHREADS] __attribute__((aligned(64)));
struct loi_phase_stats phasestats  __attribute__((aligned(64)));

// counter that holds the TSC measured frequency
uint64_t TSCFREQ;

// functionality to measure the frequency of the TSC
static uint64_t vt_pform_wtime();

// The following two functions are taken from vampirtrace 

/* wall-clock time */
// used for measuring TSC clock (note this actually replicates functionality already in loi.h)
static uint64_t __attribute__((unused)) vt_pform_wtime()
{
  uint64_t clock_value;
    /* ... TSC */
    {
      uint32_t low = 0;
      uint32_t high = 0;

      asm volatile ("rdtsc" : "=a" (low), "=d" (high));

      clock_value = ((uint64_t)high << 32) | (uint64_t)low;
    }
  return clock_value;
}

// measure the TSC frequency. This is called from the loi_init() function
//
// This function is copied almost verbatim from VampirTrace-5.14 code.
// VampirTrace is licensed under the terms of the New BSD License. 
// The copyright is held by ZIH (TU Dresden), FZJ and HLRS
static uint64_t __attribute__((unused)) cycle_counter_frequency(long);
static uint64_t __attribute__((unused)) cycle_counter_frequency(long nsleep_time)
{
    uint64_t start1_cycle_counter, start2_cycle_counter;
    uint64_t end1_cycle_counter, end2_cycle_counter;
    uint64_t start_time, end_time;
    uint64_t start_time_cycle_counter, end_time_cycle_counter;
    struct timeval timestamp;
    struct timespec nsleeptime = { 0, nsleep_time};

    if(!nsleep_time) 
        nsleep_time = 1e8;

    /* start timestamp */
    start1_cycle_counter = vt_pform_wtime();
    gettimeofday(&timestamp,NULL);
    start2_cycle_counter = vt_pform_wtime();

    start_time=timestamp.tv_sec*1000000+timestamp.tv_usec;

    nanosleep( &nsleeptime, NULL );

    /* end timestamp */
    end1_cycle_counter = vt_pform_wtime();
    gettimeofday(&timestamp,NULL);
    end2_cycle_counter = vt_pform_wtime();

    end_time=timestamp.tv_sec*1000000+timestamp.tv_usec;

    start_time_cycle_counter = (start1_cycle_counter+start2_cycle_counter)/2;
    end_time_cycle_counter   = (  end1_cycle_counter+  end2_cycle_counter)/2;

    /* freq is 1e6 * cycle_counter_time_diff/gettimeofday_time_diff */
    return (uint64_t)
             (1e6*(double)(end_time_cycle_counter-start_time_cycle_counter)/
             (double)(end_time-start_time));
}

// Back to LoI code //


// initialize LoI
// 
// This is the initialization function called from the program that is being profiled
// The tracing facility is initialized to track NUMTHREADS only
int loi_init(void)
{
   TSCFREQ = cycle_counter_frequency(5e8);
   krd_init_block(NUMTHREADS, TRACE_LENGTH);
#if DO_BENCH
   loi_bench();
   krd_bench();
#endif
   return 0; 
}

// Generate statistics 
// 'total_threads' needs to be provided by the user because LoI/KRD is not tied to any particular runtime and
// does not attempt to discover the underlying number of threads
// 'kinfo' is a user provided data structure describing the structure of kernels and the relationship to the
// program phases
int loi_statistics(struct loi_kernel_info *kinfo, int total_threads)
{
	double *ktimes; 
	double pertask_numerator = 0.0;
	double pertask_avg;
	double numerator = 0.0, stddev;
	int numthr = 0;
	int kndx = 0; // kernel index
	int ph, bitndx;
	
	ktimes = (double *) malloc (sizeof(double) * kinfo->num_kernels);
	
	for(; kndx < kinfo->num_kernels ; kndx++) ktimes[kndx] = 0.0;

// First print kernel statistics
	printf("\n\n**** Kernel Statistics **** \n\n");
	for(kndx = 0; kndx < kinfo->num_kernels; kndx++)
	{   
	int i;
	pertask_numerator = 0.0;
	numerator = 0.0;
	numthr=0;
	
	for(i = 0; i < LOI_MAXTHREADS; i++)
		if(CTR(Ktime(kndx,i)) > 0.0) 
		{ 
		numthr++;      					// add new kernel
		ktimes[kndx] += CTR(Ktime(kndx,i)); 			// add kernel time 
		printf("Kernel %s Thread %d : Total kernels %ld Total Time (s) %g Average Time (ns) %g\n",
		kinfo->kname[kndx], i, Knum(kndx,i) , CTR(Ktime(kndx,i)), 1e9 * CTR(Ktime(kndx,i)) / Knum(kndx,i));
		pertask_numerator += CTR(Ktime(kndx,i)) / Knum(kndx,i);
		} 
		pertask_avg = pertask_numerator / numthr; // this average is across the number of threads, not number of kernels
	
	// at this point we have the mean (time/numthr), so we can compute the standard deviation
	for(i = 0; i < LOI_MAXTHREADS; i++)
	  if(CTR(Ktime(kndx,i)) > 0.0) 
	 numerator += (1e9*CTR(Ktime(kndx,i))/Knum(kndx,i) - (1e9*pertask_avg))*(1e9*CTR(Ktime(kndx,i))/Knum(kndx,i) - (1e9*pertask_avg)); 
	
	// now the upper part has been computed, so we have to devide it and take the square root  
	stddev = sqrt(numerator/numthr);
	printf("Kernel %s Average %g ns, Standard Deviation %g ns, Coefficient of Variation %g %% \n", kinfo->kname[kndx],1e9*pertask_avg,stddev ,100*stddev / (1e9*pertask_avg));
	printf("Total Kernel %s time %g\n", kinfo->kname[kndx], ktimes[kndx]);
	if(numthr != total_threads) 
	  printf("Detected only %d threads\n", numthr);
	
	printf("\n");
}

	// Next print phase statistics
	printf("\n\n**** Phase Statistics **** \n\n");
	for(ph = 0; ph < kinfo->num_phases; ph++)
		{
		double accum_kernels = 0.0;
		double overhead = 0.0;
		for(bitndx = 0; bitndx < 63; bitndx++) 
			if(kinfo->phase_kernels[ph] & (((uint64_t)1)<<bitndx)) 
				accum_kernels += ktimes[bitndx];
		
		printf("%s Execution Time (sec): %g\n", kinfo->phases[ph], CTR(phasestats.exectime[ph]));
		printf("%s Overheads (sec): %g \n", kinfo->phases[ph], CTR(total_threads*phasestats.exectime[ph]) - accum_kernels);
		overhead = (CTR(total_threads*phasestats.exectime[ph]) - accum_kernels)/(CTR(total_threads*phasestats.exectime[ph]));
		printf("%s Relative Overheads (%%): %g  \n",  kinfo->phases[ph], 100*overhead);
		printf("%s Relative Inflation: %g  \n",  kinfo->phases[ph], 1 / (1-overhead));
		printf("\n");
		}
	printf("\n");
	return 0;
}

// Benchmark the overhead of profiling kernels
void loi_bench(void)
{
  
	ctimer_t tstamp, tstart, tstop;
	uint32_t coreid __attribute__((unused)), ndx;
	tstart = loi_gettime_id(&coreid);
	int bench_iterations = 1e5;
	int i;
	
	// Benchmark recording of trace and then reinitialize
	for(i = 0; i < bench_iterations; i++){
	
		tstamp = loi_gettime();
		// do nothing
		tstamp = loi_gettime_id(&coreid);
		ndx = thread_index(coreid);
		
		Ktime(0,ndx) += tstamp - tstart; // bogus time computation
		Knum(0,ndx) = Knum(0,ndx) + 1;
		}
	
	tstop = loi_gettime();
	printf("Total time for timing one kernel is %g nsec\n", 1e9*CTR((tstop-tstart)/bench_iterations));
	
	Ktime(0,ndx) = 0.0; // reset to original value
	Knum(0,ndx) = 0;

}

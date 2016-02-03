/**********************************************************************************************/
/*  This program is part of the Barcelona OpenMP Tasks Suite                                  */
/*  Copyright (C) 2009 Barcelona Supercomputing Center - Centro Nacional de Supercomputacion  */
/*  Copyright (C) 2009 Universitat Politecnica de Catalunya                                   */
/*                                                                                            */
/*  This program is free software; you can redistribute it and/or modify                      */
/*  it under the terms of the GNU General Public License as published by                      */
/*  the Free Software Foundation; either version 2 of the License, or                         */
/*  (at your option) any later version.                                                       */
/*                                                                                            */
/*  This program is distributed in the hope that it will be useful,                           */
/*  but WITHOUT ANY WARRANTY; without even the implied warranty of                            */
/*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                             */
/*  GNU General Public License for more details.                                              */
/*                                                                                            */
/*  You should have received a copy of the GNU General Public License                         */
/*  along with this program; if not, write to the Free Software                               */
/*  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA            */
/**********************************************************************************************/

/*
 *  Original code from the Cilk project
 *
 * Copyright (c) 2000 Massachusetts Institute of Technology
 * Copyright (c) 2000 Matteo Frigo
 */

/*
 * this program uses an algorithm that we call `cilksort'.
 * The algorithm is essentially mergesort:
 *
 *   cilksort(in[1..n]) =
 *       spawn cilksort(in[1..n/2], tmp[1..n/2])
 *       spawn cilksort(in[n/2..n], tmp[n/2..n])
 *       sync
 *       spawn cilkmerge(tmp[1..n/2], tmp[n/2..n], in[1..n])
 *
 *
 * The procedure cilkmerge does the following:
 *       
 *       cilkmerge(A[1..n], B[1..m], C[1..(n+m)]) =
 *          find the median of A \union B using binary
 *          search.  The binary search gives a pair
 *          (ma, mb) such that ma + mb = (n + m)/2
 *          and all elements in A[1..ma] are smaller than
 *          B[mb..m], and all the B[1..mb] are smaller
 *          than all elements in A[ma..n].
 *
 *          spawn cilkmerge(A[1..ma], B[1..mb], C[1..(n+m)/2])
 *          spawn cilkmerge(A[ma..m], B[mb..n], C[(n+m)/2 .. (n+m)])
 *          sync
 *
 * The algorithm appears for the first time (AFAIK) in S. G. Akl and
 * N. Santoro, "Optimal Parallel Merging and Sorting Without Memory
 * Conflicts", IEEE Trans. Comp., Vol. C-36 No. 11, Nov. 1987 .  The
 * paper does not express the algorithm using recursion, but the
 * idea of finding the median is there.
 *
 * For cilksort of n elements, T_1 = O(n log n) and
 * T_\infty = O(log^3 n).  There is a way to shave a
 * log factor in the critical path (left as homework).
 */

#if TBB 
#include <tbb/task_group.h>
#include "tbb/task_scheduler_init.h"
#include "tbb/tbb.h"
using namespace tbb; // for task_scheduler_init
#endif

#include <chrono>
#include <thread>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <iostream>
#include <atomic>

using namespace std;

// NOTE: because a collision of cilksort "array" and TBB namespace "struct array", 
// I have renamed "array" to "sort_array"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SORT_BUFFER_SIZE (32*1024*1024)
//#define MERGE_TASK_SIZE (1*1024)
#define SORT_TASK_SIZE (16*3*1024)
#define INSERTION_THR (20)

#include "../config.h"

#ifdef DO_LOI
#include "loi.h"

// keep exact same format as LOI example 
enum kernels{
    MERGE = 0, 
    QSORT = 1, // this is libc qsort(), it does not necessarily implement quicksort
    };

/* this structure describes the relationship between phases and kernels in the application */ 
struct loi_kernel_info sort_kernels = {
        2,              // 2 kernels in total
        1,              // 1 phase
        {"Merge_Core", "QSort_Core"}, // Name of the two kernels
        {"Sort"},                     // Name of the phase
        {(1<<MERGE | 1<<QSORT)}, // both kernels belong to the same phase [0]
                                 // if additional phases exist, just add an additional bitmask after a comma 
        };

#endif

typedef long ELM;

void seqquick(ELM *low, ELM *high);
void seqmerge(ELM *low1, ELM *high1, ELM *low2, ELM *high2, ELM *lowdest);
ELM *binsplit(ELM val, ELM *low, ELM *high);
void cilkmerge(ELM *low1, ELM *high1, ELM *low2, ELM *high2, ELM *lowdest, int);
void cilksort(ELM *low, ELM *tmp, long size);
void scramble_sort_array( void );
void fill_sort_array( void );
void sort ( void );
void sort_par ( void );
void sort_init ( void );
int  sort_verify ( void );
void print_sort_array( int, ELM * );

int sort_buffer_size = SORT_BUFFER_SIZE;
int sort_task_size = SORT_TASK_SIZE;
int insertion_thr = INSERTION_THR;

static unsigned long rand_nxt = 0;

static inline unsigned long my_rand(void)
{
     rand_nxt = rand_nxt * 1103515245 + 12345;
     return rand_nxt;
}

static inline void my_srand(unsigned long seed)
{
     rand_nxt = seed;
}

static inline ELM med3(ELM a, ELM b, ELM c)
{
     if (a < b) {
	  if (b < c) {
	       return b;
	  } else {
	       if (a < c)
		    return c;
	       else
		    return a;
	  }
     } else {
	  if (b > c) {
	       return b;
	  } else {
	       if (a > c)
		    return c;
	       else
		    return a;
	  }
     }
}

/*
 * simple approach for now; a better median-finding
 * may be preferable
 */
static inline ELM choose_pivot(ELM *low, ELM *high)
{
     return med3(*low, *high, low[(high - low) / 2]);
}

static ELM *seqpart(ELM *low, ELM *high)
{
     ELM pivot;
     ELM h, l;
     ELM *curr_low = low;
     ELM *curr_high = high;

     pivot = choose_pivot(low, high);

     while (1) {
	  while ((h = *curr_high) > pivot)
	       curr_high--;

	  while ((l = *curr_low) < pivot)
	       curr_low++;

	  if (curr_low >= curr_high)
	       break;

	  *curr_high-- = l;
	  *curr_low++ = h;
     }

     /*
      * I don't know if this is really necessary.
      * The problem is that the pivot is not always the
      * first element, and the partition may be trivial.
      * However, if the partition is trivial, then
      * *high is the largest element, whence the following
      * code.
      */
     if (curr_high < high)
	  return curr_high;
     else
	  return curr_high - 1;
}

#define swap(a, b) \
{ \
  ELM tmp;\
  tmp = a;\
  a = b;\
  b = tmp;\
}

static void insertion_sort(ELM *low, ELM *high)
{
     ELM *p, *q;
     ELM a, b;

     for (q = low + 1; q <= high; ++q) {
	  a = q[0];
	  for (p = q - 1; p >= low && (b = p[0]) > a; p--)
	       p[1] = b;
	  p[1] = a;
     }
}

/*
 * tail-recursive quicksort, almost unrecognizable :-)
 */
void seqquick(ELM *low, ELM *high)
{
     ELM *p;

     while (high - low >= insertion_thr) {
	  p = seqpart(low, high);
	  seqquick(low, p);
	  low = p + 1;
     }

     insertion_sort(low, high);
}

void seqmerge(ELM *low1, ELM *high1, ELM *low2, ELM *high2, ELM *lowdest)
{
     ELM a1, a2;

     /*
      * The following 'if' statement is not necessary
      * for the correctness of the algorithm, and is
      * in fact subsumed by the rest of the function.
      * However, it is a few percent faster.  Here is why.
      *
      * The merging loop below has something like
      *   if (a1 < a2) {
      *        *dest++ = a1;
      *        ++low1;
      *        if (end of sort_array) break;
      *        a1 = *low1;
      *   }
      *
      * Now, a1 is needed immediately in the next iteration
      * and there is no way to mask the latency of the load.
      * A better approach is to load a1 *before* the end-of-sort_array
      * check; the problem is that we may be speculatively
      * loading an element out of range.  While this is
      * probably not a problem in practice, yet I don't feel
      * comfortable with an incorrect algorithm.  Therefore,
      * I use the 'fast' loop on the sort_array (except for the last 
      * element) and the 'slow' loop for the rest, saving both
      * performance and correctness.
      */

     if (low1 < high1 && low2 < high2) {
	  a1 = *low1;
	  a2 = *low2;
	  for (;;) {
	       if (a1 < a2) {
		    *lowdest++ = a1;
		    a1 = *++low1;
		    if (low1 >= high1)
			 break;
	       } else {
		    *lowdest++ = a2;
		    a2 = *++low2;
		    if (low2 >= high2)
			 break;
	       }
	  }
     }
     if (low1 <= high1 && low2 <= high2) {
	  a1 = *low1;
	  a2 = *low2;
	  for (;;) {
	       if (a1 < a2) {
		    *lowdest++ = a1;
		    ++low1;
		    if (low1 > high1)
			 break;
		    a1 = *low1;
	       } else {
		    *lowdest++ = a2;
		    ++low2;
		    if (low2 > high2)
			 break;
		    a2 = *low2;
	       }
	  }
     }
     if (low1 > high1) {
	  memcpy(lowdest, low2, sizeof(ELM) * (high2 - low2 + 1));
     } else {
	  memcpy(lowdest, low1, sizeof(ELM) * (high1 - low1 + 1));
     }
}

#define swap_indices(a, b) \
{ \
  ELM *tmp;\
  tmp = a;\
  a = b;\
  b = tmp;\
}

ELM *binsplit(ELM val, ELM *low, ELM *high)
{
     /*
      * returns index which contains greatest element <= val.  If val is
      * less than all elements, returns low-1
      */
     ELM *mid;

     while (low != high) {
	  mid = low + ((high - low + 1) >> 1);
	  if (val <= *mid)
	       high = mid - 1;
	  else
	       low = mid;
     }

     if (*low > val)
	  return low - 1;
     else
	  return low;
}

void cilkmerge(ELM *low1, ELM *high1, ELM *low2, ELM *high2, ELM *lowdest, int recurse)
{
     /*
      * Cilkmerge: Merges range [low1, high1] with range [low2, high2] 
      * into the range [lowdest, ...]  
      */

     ELM *split1, *split2;	/*
				 * where each of the ranges are broken for 
				 * recursive merge 
				 */
     long int lowsize;		/*
				 * total size of lower halves of two
				 * ranges - 2 
				 */
     /*
      * We want to take the middle element (indexed by split1) from the
      * larger of the two sort_arrays.  The following code assumes that split1
      * is taken from range [low1, high1].  So if [low1, high1] is
      * actually the smaller range, we should swap it with [low2, high2] 
      */

     if (high2 - low2 > high1 - low1) {
	  swap_indices(low1, low2);
	  swap_indices(high1, high2);
     }
     if (high2 < low2) {
	  /* smaller range is empty */
	  memcpy(lowdest, low1, sizeof(ELM) * (high1 - low1));
	  return;
     }
     if (!recurse) {

#if DO_LOI
#ifdef KRD_MULTI_PARTITION
        long int dest_elems = (high1 - low1 + high2 - low2 + 2);
        long int partitions = ((dest_elems-1) / MERGEBLOCK) + 1;
        long int ip;
        long int dest_part = dest_elems / partitions;
        long int size1p = (high1 - low1 + 1) / partitions;
        long int size2p = dest_part - size1p;
#endif
        kernel_profile_start();
#endif
        seqmerge(low1, high1, low2, high2, lowdest);
#if DO_LOI
        kernel_profile_stop(MERGE);
#if DO_KRD
#ifdef KRD_MULTI_PARTITION
    for(ip = 0; (dest_elems - dest_part*ip) >= dest_part ; ip++ )
    {
//            printf("recording address %p, elems %ld\n", low1 + ip*size1p, KREAD(size1p)); 
        kernel_trace3(MERGE,
                 low1 + ip*size1p, KREAD(size1p)*sizeof(ELM),
                 low2 + ip*size2p, KREAD(size2p)*sizeof(ELM),
                 lowdest + ip*dest_part, KWRITE(dest_part)*sizeof(ELM));
    }

//    printf("%d iterations\n", ip);
//    {
//   // now process the residual part
   if(  (dest_elems - dest_part*ip) > 0){
          //  printf("recording address %p, elems %ld\n", low1 + ip*size1p, KREAD(high1 - low1 + 1 - ip*size1p)); 
            kernel_trace3(MERGE,
                 low1 + ip*size1p,       KREAD(high1 - low1 + 1 - ip*size1p) * sizeof(ELM),
                 low2 + ip*size2p,       KREAD(high2 - low2 + 1 - ip*size2p) * sizeof(ELM),
                 lowdest + ip*dest_part, KWRITE(dest_elems - ip*dest_part)*sizeof(ELM));
   }

#else 
        kernel_trace3(MERGE,
                 low1, KREAD(high1 - low1 + 1)*sizeof(ELM),
                 low2, KREAD(high2 - low2 + 1)*sizeof(ELM),
                 lowdest, KWRITE(high1 - low1 + high2 - low2 + 1)*sizeof(ELM));
#endif
#endif
#endif
	  return;
     }
     /*
      * Basic approach: Find the middle element of one range (indexed by
      * split1). Find where this element would fit in the other range
      * (indexed by split 2). Then merge the two lower halves and the two
      * upper halves. 
      */

     split1 = ((high1 - low1 + 1) / 2) + low1;
     split2 = binsplit(*split1, low2, high2);
     lowsize = split1 - low1 + split2 - low2;

     /* 
      * directly put the splitting element into
      * the appropriate location
      */
     *(lowdest + lowsize + 1) = *split1;
#if TBB // TBB tasks expressed with lambdas
    task_group tg;
    tg.run([=] { cilkmerge(low1, split1 - 1, low2, split2, lowdest, 0); });
    tg.run([=] { cilkmerge(split1 + 1, high1, split2 + 1, high2, lowdest+lowsize+2, 0); });
    tg.wait();
#else   // sequential version
     cilkmerge(low1, split1 - 1, low2, split2, lowdest, 0);
     cilkmerge(split1 + 1, high1, split2 + 1, high2, lowdest+lowsize+2, 0);
#endif

     return;
}

void cilksort(ELM *low, ELM *tmp, long size)
{
     /*
      * divide the input in four parts of the same size (A, B, C, D)
      * Then:
      *   1) recursively sort A, B, C, and D (in parallel)
      *   2) merge A and B into tmp1, and C and D into tmp2 (in parallel)
      *   3) merbe tmp1 and tmp2 into the original sort_array
      */
     long quarter = size / 4;
     ELM *A, *B, *C, *D, *tmpA, *tmpB, *tmpC, *tmpD;

     if (size < sort_task_size) {
#if DO_LOI
        kernel_profile_start();
#endif
        seqquick(low, low + size - 1);
#if DO_LOI
        kernel_profile_stop(QSORT);
#if DO_KRD
        kernel_trace1(QSORT, low, KWRITE(size - 1)*sizeof(ELM));
#endif
#endif
	  return;
     }
     A = low;
     tmpA = tmp;
     B = A + quarter;
     tmpB = tmpA + quarter;
     C = B + quarter;
     tmpC = tmpB + quarter;
     D = C + quarter;
     tmpD = tmpC + quarter;
     
#if TBB
     {
     task_group tg;
     tg.run([=] { cilksort(A, tmpA, quarter);             });
     tg.run([=] { cilksort(B, tmpB, quarter);             });
     tg.run([=] { cilksort(C, tmpC, quarter);             });
     tg.run([=] { cilksort(D, tmpD, size - 3 * quarter);  });
     tg.wait();
     }
#else
     cilksort(A, tmpA, quarter);            
     cilksort(B, tmpB, quarter);
     cilksort(C, tmpC, quarter);
     cilksort(D, tmpD, size - 3 * quarter);
#endif

#if TBB
     {
     task_group tg;
     tg.run([=] { cilkmerge(A, A + quarter - 1, B, B + quarter - 1, tmpA, 1); });
     tg.run([=] { cilkmerge(C, C + quarter - 1, D, low + size - 1, tmpC, 1);  });
     tg.wait();
     }
#else
     cilkmerge(A, A + quarter - 1, B, B + quarter - 1, tmpA, 1);
     cilkmerge(C, C + quarter - 1, D, low + size - 1, tmpC, 1);
#endif

     cilkmerge(tmpA, tmpC - 1, tmpC, tmpA + size - 1, A, 1);
}

ELM *sort_array, *tmp;

void scramble_sort_array( void )
{
     unsigned long i;
     unsigned long j;

     for (i = 0; i < sort_buffer_size; ++i) {
	  j = my_rand();
	  j = j % sort_buffer_size;
	  swap(sort_array[i], sort_array[j]);
     }
}

void fill_sort_array( void )
{
     unsigned long i;

     my_srand(1);

     /* first, fill with integers 1..size */
     for (i = 0; i < sort_buffer_size; ++i) {
	  sort_array[i] = i;
     }
}

/*
int sort_buffer_size = SORT_BUFFER_SIZE;
int sort_task_size = SORT_TASK_SIZE;
int insertion_thr = INSERTION_THR;
*/


int main ( int argc, char *argv[] )
{
        if(argc == 0) printf("Usage: ./sort <buffer_size> <sort_size> <insertion_threshold>\n");
        if(argc > 1) sort_buffer_size = atoi(argv[1]);
        if(argc > 2) sort_task_size = atoi(argv[2]);
        if(argc > 3) insertion_thr = atoi(argv[4]);

        std::chrono::time_point<std::chrono::system_clock> start, end;
#ifdef TBB
    // initialize threads
    int tbb_nthreads;
    if(getenv("TBB_NTHREADS"))
        tbb_nthreads = atoi(getenv("TBB_NTHREADS"));
    else 
        tbb_nthreads = task_scheduler_init::automatic;
    task_scheduler_init init(tbb_nthreads);
#endif

        sort_array = (ELM *) malloc(sort_buffer_size * sizeof(ELM));
        tmp = (ELM *) malloc(sort_buffer_size * sizeof(ELM));

        fill_sort_array();
        scramble_sort_array();

        std::cout << "Sorting " << sort_buffer_size << " integers via 4-1 TBBSort " << std::endl;   

#if DO_LOI 
    loi_init(); // calc TSC freq and init data structures
    printf(" TSC frequency has been measured to be: %g Hz\n", (double) TSCFREQ);
    int maxthr;

    if (tbb_nthreads > 0)
        maxthr = tbb_nthreads;
    else
        maxthr =  task_scheduler_init::default_num_threads();
#else
    int maxthr = 1;
#endif

        start = std::chrono::system_clock::now();
#ifdef DO_LOI 
    phase_profile_start();
#endif
#if TBB
    task_group tg;

    // start multisort
    tg.run([=] { cilksort(sort_array, tmp, sort_buffer_size); });
    tg.wait();
#else
        cilksort(sort_array, tmp, sort_buffer_size);
#endif
#ifdef DO_LOI
    phase_profile_stop(0); 
#endif
        
    end = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds = end-start;
    std::time_t end_time = std::chrono::system_clock::to_time_t(end);

    std::cout << "elapsed time: " << elapsed_seconds.count() << "s\n";

        if(!sort_verify ( )) {
                printf("Sort Failed!\n"); 
                return 1;
        }

        printf("Sort was successful!\n"); 
#ifdef DO_LOI
#ifdef LOI_TIMING
    loi_statistics(&sort_kernels, maxthr);
#endif
#ifdef DO_KRD
    krd_save_traces();
#endif
#endif
        return 0;
}


int sort_verify ( void )
{
     int i, success = 1;
     for (i = 0; i < sort_buffer_size; ++i)
	  if (sort_array[i] != i) success = 0;

     return success ;
}

void print_sort_array( int a, ELM *arr)
{
        if(a < 1) return;
        if(a > 100) a = 100;
        for(int i =0; i < a; i++) 
                printf("Elem %d = %ld\n", i, arr[i]);
}


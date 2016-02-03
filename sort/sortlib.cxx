// tao_objs.cxx
//
//


#include "taosort.h"

// Use LOI instrumentation: 
#ifdef DO_LOI

/* this structure describes the relationship between phases and kernels in the application */ 
struct loi_kernel_info sort_kernels = {
        2,              // 2 kernels in total
        1,              // 1 phase
        {"Merge_Core", "QSort_Core"}, // Name of the two kernels
        {"Sort"},                     // Name of the phase
        {(1<<MERGE | 1<<QSORT)}, // both kernels belong to the same phase [0]
        };

#endif

int sort_buffer_size = SORT_BUFFER_SIZE;
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
      *        if (end of array) break;
      *        a1 = *low1;
      *   }
      *
      * Now, a1 is needed immediately in the next iteration
      * and there is no way to mask the latency of the load.
      * A better approach is to load a1 *before* the end-of-array
      * check; the problem is that we may be speculatively
      * loading an element out of range.  While this is
      * probably not a problem in practice, yet I don't feel
      * comfortable with an incorrect algorithm.  Therefore,
      * I use the 'fast' loop on the array (except for the last 
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


ELM *array, *tmp;

void scramble_array( void )
{
     unsigned long i;
     unsigned long j;

     for (i = 0; i < sort_buffer_size; ++i) {
	  j = my_rand();
	  j = j % sort_buffer_size;
	  swap(array[i], array[j]);
     }
}

void fill_array( void )
{
     unsigned long i;

     my_srand(1);

     /* first, fill with integers 1..size */
     for (i = 0; i < sort_buffer_size; ++i) {
	  array[i] = i;
     }
}



void quicksort(int tid, struct seqquick_uow *arg)
{
#if DO_LOI
    kernel_profile_start();
#endif

    seqquick(arg->low, arg->high);

#if DO_LOI
    kernel_profile_stop(QSORT);
#if DO_KRD
    kernel_trace1(QSORT, arg->low, KWRITE(arg->high - arg->low)*sizeof(ELM));
#endif
#endif
}

void mergesort(int tid, struct seqmerge_uow *arg)
{

#if DO_LOI
#ifdef KRD_MULTI_PARTITION
    long int dest_elems = (arg->high1 - arg->low1 + arg->high2 - arg->low2 + 2);
    long int partitions =  ((dest_elems-1) / MERGEBLOCK) + 1;
    long int ip;
    long int dest_part = dest_elems / partitions;
    long int size1p = (arg->high1 - arg->low1 + 1) / partitions;
    long int size2p = dest_part - size1p;

    //printf("dest_part %d, dest_elems %d, partitions %d "
    //       " (%d-1 / %d) = %d \n", 
    //        dest_part, dest_elems, partitions, dest_elems, MERGEBLOCK, (dest_elems-1 / MERGEBLOCK));
#endif
    kernel_profile_start();
#endif

    seqmerge(arg->low1, arg->high1, arg->low2, arg->high2, arg->lowdest);

#if DO_LOI
    kernel_profile_stop(MERGE);
#if DO_KRD
#ifdef KRD_MULTI_PARTITION
    for(ip = 0; (dest_elems - dest_part*ip) >= dest_part ; ip++ ){
            kernel_trace3(MERGE,
                 arg->low1 + ip*size1p, KREAD(size1p)*sizeof(ELM),
                 arg->low2 + ip*size2p, KREAD(size2p)*sizeof(ELM),
                 arg->lowdest + ip*dest_part, KWRITE(dest_part)*sizeof(ELM));
    }
   // now process the residual part
   if(  (dest_elems - dest_part*ip) > 0)
            kernel_trace3(MERGE,
                 arg->low1 + ip*size1p,       KREAD(arg->high1 - arg->low1 - ip*size1p + 1)*sizeof(ELM),
                 arg->low2 + ip*size2p,       KREAD(arg->high2 - arg->low2 - ip*size2p + 1)*sizeof(ELM),
                 arg->lowdest + ip*dest_part, KWRITE(dest_elems - ip*dest_part)*sizeof(ELM));

#else
        kernel_trace3(MERGE,
                 arg->low1, KREAD(arg->high1 - arg->low1 + 1)*sizeof(ELM),
                 arg->low2, KREAD(arg->high2 - arg->low2 + 1)*sizeof(ELM),
                 arg->lowdest, KWRITE(arg->high1 - arg->low1 + arg->high2 - arg->low2 + 2)*sizeof(ELM));

#endif    
#endif
#endif
}

// question: how to communicate the values between different UOWs?
// This should be resolved via the DOW, in order to keep all communication local
// It should be possible for functions to modify values that are parameters of later UOWs.
// This can be done by setting up pointers to those entries

void binpart(int tid, struct binsplit_uow *arg)
{
        ELM *split1, *split2;
        long int lowsize;
        long fourth = arg->size / 4;
  
        split1 = fourth + arg->regA;
        split2 = binsplit(*split1, arg->regB, arg->regB + 2*fourth-1);
        lowsize = split1 - arg->regA + split2 - arg->regB;
   
        /* 
         * directly put the splitting element into
         * the appropriate location
         */
        *(arg->target + lowsize + 1) = *split1;
   
//        printf("element %ld at entry %ld\n", *split1, lowsize + 1);
        // now update the dynamic outputs
        *arg->out_low_high2  = split2;
        *arg->out_up_low2    = split2 + 1;
        *arg->out_up_lowdest = arg->target + lowsize + 2;

//        std::cout << "Thread " << tid << " binsplit high2: " << split2 - array << std::endl;
//        std::cout << "Thread " << tid << " binsplit low2: "  << split2+1 - array << std::endl;
//        std::cout << "Thread " << tid << " binsplit lowdest: " << lowsize + 2 << std::endl;
}


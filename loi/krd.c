/* krd.c
 * Copyright 2014 Miquel Pericas and Tokyo Institute of Technology
 * This program is licensed under the terms of the New BSD License.
 * See LICENSE for full details
 *
 * krd.c: Kernel Reuse Distance Method
 * This file contains functionality necessary for processing of traces and generation of histograms
 * For trace generation check the file krd_trace.c
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


// NAIVE STACK DISTANCE ALGORITHM IMPLEMENTATION

// Stack data structure
struct stack_entry{
  uint64_t elem; // element identifier (sparse) or address (array)
  uint32_t size; // size in bytes associated with this identifier
  int     inv;   // invalid entry ("hole") or regular entry
  short   core;  // identifies the core that accessesed or invalidated this block
  struct stack_entry *next;
  struct stack_entry *prev;
  };


#define HISTOGRAM_LEN   7    // log range of histogram (2^HL entries)

// histograms, cold misses, and invalidation misses
// For histogram computation only one array out of LOI_MAXTHREADS is used
// [LOI_MAXTHREADS] arrays are used by the ACCUM tool
static long long hist_distance_vector[LOI_MAXTHREADS][1<<HISTOGRAM_LEN];    // histogram
static long long hist_shared_vector[LOI_MAXTHREADS][1<<HISTOGRAM_LEN];      // tracks reuses that come from a different thread 
static long long hist_inv_miss_vector[LOI_MAXTHREADS][1<<HISTOGRAM_LEN];    // tracks distance of misses that occur due to invalidations 
static long long hist_cold_misses[LOI_MAXTHREADS];   // cold misses are data references seen for the first time
static long long hist_inv_misses[LOI_MAXTHREADS];    // counts misses due to invalidations from a different thread

// the cummulative histograms:
static long long hist_total_distance_vector[1<<HISTOGRAM_LEN]; // cummulative histogram
static long long hist_total_shared_vector[1<<HISTOGRAM_LEN];   // shared reuses 
static long long hist_total_inv_vector[1<<HISTOGRAM_LEN];      // distance of all invalidation misses
static long long hist_total_cold_misses;               // cummulative cold misses
static long long hist_total_inv_misses;                // cummulative invalidation misses
static long long total_refs;                       // total references

// Head pointer to the stack structure. Empty upon initialization
static struct stack_entry *stack_head = NULL;


// Debug function
// Useful within gdb to print the contents of the complete stack, but otherwise unused
 void hist_print_debug()
{
  int i = 0;
  struct stack_entry *entry = stack_head;

  while(entry){
    printf("Entry %d: %p: Element %lu, Size %d, Invalid %d, Next %p, Prev %p\n", i, entry, entry->elem, entry->size, entry->inv, entry->next, entry->prev);
    entry = entry->next; i++;
    if( i > 1000000) break; // safeguard. If there is a loop, stop after printing 1e6 elements
   }
}

// simple test to check consistency of ->next and ->prev pointers
// Active only if DEBUG is defined. Histogram generation is noticeably slowed down by this check if called for every element
 void check_trace_consistency(int iteration)
{
  int i = 0;
  struct stack_entry *entry = stack_head;

  while(entry){
   if(entry->next && (entry->next->prev != entry)) printf("Detected inconsistent trace at iteration %d\n", iteration);
   entry = entry->next; i++;
   if( i > 1000000) break; // safeguard in case there is a loop
   }
}

// count total data on the stack and print it
// this computes an approximation of the value "M" corresponding to the total amount of distinct addresses
static void print_trace_data()
{
  struct stack_entry *entry = stack_head;
  unsigned long data = 0;

  while(entry){
   data += abs(entry->size);
   entry = entry->next;
   }

  printf("Total Stack Data is %lu bytes\n", data);
}

// check if data blocks overlap and record overlap region in 'start' and 'size'

// For Vector data (default) this is computable
// For Sparse data this is not possible without further details on data layout

static int overlap(int model,
        uint64_t stackaddr, uint32_t stacksize __attribute__((unused)),
        uint64_t inaddr, uint32_t insize __attribute__((unused)),
        uint64_t *start, uint32_t *size __attribute__((unused)))
{
if((model == SPARSE) || (model == ARRAY_DISJOINT)){
  if(inaddr == stackaddr) { 
     *start = inaddr;
     *size = insize;
     return 1;
     }
  return 0;
} 
else // model == ARRAY_PARTIAL
{
  if(((inaddr+insize) > stackaddr) && (inaddr < (stackaddr+stacksize))){
    *start = (stackaddr > inaddr)? stackaddr : inaddr;
    *size = ((stackaddr+stacksize) < (inaddr+insize))? (stackaddr+stacksize) - *start :  (inaddr+insize)  - *start;
    return 1;
   }
  else return 0;
}
}


// check if target element is a superset of the current entry in the stack
static inline int superset(struct stack_entry *sentry, uint64_t element, uint32_t size)
{
  return ((sentry->elem >= element) && (((sentry->elem + sentry->size) <= (element + size))));
}

// compute base 2 ceiling
// code snippet from http://stackoverflow.com/questions/3272424/compute-fast-log-base-2-ceiling
// CC BY-SA 2.5 applies to this function. See http://creativecommons.org/licenses/by-sa/2.5/legalcode. No changes have been made
static inline int ceil_log2(unsigned long long x)
{
  static const unsigned long long t[6] = {
    0xFFFFFFFF00000000ull,
    0x00000000FFFF0000ull,
    0x000000000000FF00ull,
    0x00000000000000F0ull,
    0x000000000000000Cull,
    0x0000000000000002ull
  };

  int y = (((x & (x - 1)) == 0) ? 0 : 1);
  int j = 32;
  int i;

  for (i = 0; i < 6; i++) {
    int k = (((x & t[i]) == 0) ? 0 : j);
    y += k;
    x >>= k;
    j >>= 1;
  }

  return y;
}

// check if element is a regular access or an invalidation 
static inline int is_invalidation_probe(int32_t rsize)
{
  // no invalidations can happen in RAW traces
  if(krd_trace_magic == RAW_TRACE_MAGIC)
    return ELEM_RDWR;

  else if(krd_trace_magic == RWI_TRACE_MAGIC){
    if(rsize >= 0) return ELEM_RDWR;
    if(rsize <  0) return ELEM_INV;
    } 
  else printf("There is a problem with the magic number. Please check the trace\n"); 
  exit(1);
}



// Bloom Filter to accelerate the case of cold misses (SPARSE and ARRAY_DISJOINT models)
// Cold misses are just few, but since they run through the whole list their impact on performance is not negligible.
// This is particularly useful for the case of invalidations, which often run through the queue without invalidating any data


// length of filter table. By default this size is quite large, so make sure that you have the memory
// Depending on the trace, a smaller hash can provide good results as well 
#define HASH_SIZE  ((1<<28)+1)

static uint8_t *bloom_hash;  // allocate only if used

// hash function
// it should distribute input addresses randomly into table entries 
#define hash_addr(a) ((a>>3)%HASH_SIZE)


// memory debugging facility
// If MEM_DEBUG is defined, print a trace of all the node allocations
// NOTE: This is extremely verbose
#ifdef MEM_DEBUG
#define krd_malloc(size) \
({ void *ptr = malloc(size); \
   printf("Malloc: %p, %ld at %s:%d\n", ptr, size, __FILE__, __LINE__); \
   ptr; }) 

#define krd_free(ptr) \
({ printf("Free:   %p at %s:%d\n", ptr, __FILE__, __LINE__); \
   free(ptr); })

#else

#define krd_malloc(size) malloc(size); 

#define krd_free(ptr) free(ptr); 

#endif

// KRD HISTOGRAM GENERATION

// Generate on histogram using a flat memory model where element ids are addresses
// Partial matches are handled for Vector model
//
// Stack warm-up and partition size are handled to support the parallel strategy
//

void do_one_histogram_mem(
		int data_model,				// data model
		int threadid,				// thread ID
		unsigned int warmup,		// starting point of warmup phase
		unsigned int start_histo,	// starting point of histogram generation (start > warmup)
		unsigned long partition, // size of the partition. Used to determine finalization
		int16_t kid) 		// kernel id that is to be tracked. note it must be signed integer. "-1" means all
{ 
    // performance measurement of histogram generation
    struct timespec __tsstart, __tsstop; 
    double __timelapse; 
    clock_gettime(CLOCK_MONOTONIC, &__tsstart);
  
	unsigned int i, percent_complete = 0;
	unsigned long long int visited_nodes = 0;
	unsigned int repartitions = 0, repartitioned = 0;
	int warmup_phase = 1; // start
	unsigned long total_bytes_processed = 0;
	FILE * log;
	char logname[100];
	struct rusage resource_usage;

	if((data_model == ARRAY_DISJOINT) || (data_model == SPARSE))
		bloom_hash = (uint8_t *) calloc(HASH_SIZE, sizeof(uint8_t)); 

	sprintf(logname, "krd_log_%d_%u_%u_%lu.log", threadid, warmup, start_histo, partition);
	if(partition == -1)
		log = stdout;
	else{ 
		log = fopen(logname, "w");
		if(!log) printf("Error trying to open log file %s\n", logname);
		}

	fprintf(log, "Generating histogram for %d", threadid); 

	// Here starts computation of the reuse histogram
	if(warmup >= start_histo)
		warmup_phase = 0; // do not run a warmup phase

	for(i = warmup; i < TTrace_num_elems(threadid); i++){
		// Got an element
		struct krd_id_tsc * next = threadtraces[threadid].data_ids+i;
		uint64_t element  = next->id;	  // element address
		int32_t rsize	  = next->size;   // element raw size
		short  core	  = next->core;
		uint16_t elem_kid = next->kernel_id;
		// short numa;	 // sharing data inter-NUMA is not currently supported

		int inv_probe = is_invalidation_probe(rsize); // is it an invalidation?
		uint32_t size = abs(rsize);					  // size in bytes

		uint32_t match_bytes_pending = size;	 // bytes that have not been matched yet (including invalidations)
		uint32_t reuse_distance = 0;			 // reuse distance in bytes (approximation bounded by size of block)
		struct stack_entry *sentry = stack_head; 

		uint64_t base_addr = element;
		uint32_t base_size = size;

		if(warmup_phase && (i > start_histo)) {
			fprintf(log, "End of warmup phase at %u\n", i);
			warmup_phase = 0;
			}

		// Regular path (not invalidation)
		if(!inv_probe){

			if(!warmup_phase){
				total_bytes_processed += size;
				if(total_bytes_processed > partition) // we're done
					break; // exit loop
				}

			if((data_model == ARRAY_DISJOINT) || (data_model == SPARSE)){
				// check bloom filter
				if(!bloom_hash[hash_addr(element)]){ 
					sentry = NULL; // this will trigger allocation of the element below
				if(!warmup_phase) 
					bloom_hash[hash_addr(element)]=1;
					}
				}

			while(sentry){
				uint64_t start;  // starting position of current match
				uint32_t bytes;  // size of current match
				uint32_t matching; // matching bytes
				uint32_t bounded_distance; // worst case reuse for current block and matching block.

				reuse_distance += sentry->size; 
				visited_nodes++; 
				bounded_distance = reuse_distance > size? reuse_distance : size;

				// compute current overlap
				if(overlap(data_model, sentry->elem, sentry->size, element, size, &start, &bytes)){
					match_bytes_pending -= bytes;

				// how many bytes do really belong the the current element?
				matching = bytes;
				if (start < base_addr) matching -= (base_addr-start);
				if ((start+bytes) > (base_addr+base_size)) matching -= ((start+bytes)-(base_addr+base_size));

				// increment histogram for the current match if tracking is enabled for this kernel
				if(sentry->inv && !warmup_phase && ((kid == -1) || (kid == elem_kid)) ){ // we hit an invalidation. add cold invalidation miss
					hist_inv_misses[threadid]  += matching / 4;
					hist_cold_misses[threadid] += matching / 4;
					// check also the distance at which this invalidation miss happened
					hist_inv_miss_vector[threadid][ceil_log2(bounded_distance)] += matching / 4;
					} 
				// otherwise this is a regular hit, so we compute its reuse distance
				else if(!warmup_phase && ((kid == -1) || (kid == elem_kid))){ 
					hist_distance_vector[threadid][ceil_log2(bounded_distance)] += matching / 4;
					if(sentry->core != core) 
						hist_shared_vector[threadid][ceil_log2(bounded_distance)] += matching / 4;
					}

				// now update the element in the stack
				// if an invalidation was found, then it is correct to set status to valid and put it in the front of the stack
				if(superset(sentry, start, bytes)){ // if superset, then remove this entry 
					struct stack_entry *matched;
					// although it would be possible to just move the element to the front, to simplify here we just move the 
					// elements off the stack and insert them later
					if(sentry != stack_head){ 	// If it is not the stack head
						sentry->prev->next = sentry->next; // remove element from current position
						if(sentry->next)	 	// if not last element
							sentry->next->prev = sentry->prev;
						}
					else { // if is is the stack head
						stack_head = sentry->next;
						if(sentry->next) // if not last element
							sentry->next->prev = NULL;
						}
					matched = sentry; 
					sentry = sentry->prev; 	// go back to the previous, but sentry could be NULL so take care
					krd_free(matched);		// krd_free this element, as it can no longer be found
					}
				else{  // if not superset
					// when the match is not perfect, it gets trickier
					// the most complex case is if the match is an interior subset of the element found in the stack
					// this case is handled first
					repartitioned=1;
					if((start > sentry->elem) && ((start+bytes) < (sentry->elem + sentry->size))){
						// split the elements into a pre_part and a post_part and update the elements
						struct stack_entry *post_part = (struct stack_entry *) krd_malloc(sizeof(*post_part));
						struct stack_entry *pre_part = sentry;
						// first update values
						post_part->elem = start + bytes;
						post_part->size = (sentry->elem + sentry->size) - post_part->elem;
						post_part->core = sentry->core; 
						post_part->inv = sentry->inv;  // copy invalid bit
						//pre_part->elem = sentry->elem; // redundant
						//pre_part->inv = sentry->inv;	 // redundant 
						//pre_part->core = sentry->core; // redundant
						pre_part->size = start-sentry->elem; // update size of invalid region
						// now update pointers
						post_part->next = sentry->next;
						post_part->prev = pre_part;
						pre_part->next = post_part;
						// pre_part->prev does not change
						if(post_part->next) // update element after post_part if not the tail
							post_part->next->prev = post_part;
						}
						// if the start matches
					else if(sentry->elem == start){
						// modify the size of the element
						sentry->size = sentry->size - bytes;
						sentry->elem = start+bytes;
						}
					else // if the match is at the end
						sentry->size = sentry->size - bytes;
					}  // end superset check
				} // end overlap check
				if(match_bytes_pending == 0){ // we are done. put the element on top and break the loop
					struct stack_entry *new_head = (struct stack_entry *) krd_malloc(sizeof(*new_head));
					new_head->next = stack_head; 
					new_head->prev = NULL;
					new_head->elem = element;
					new_head->size = size;
					new_head->core = core;
					new_head->inv = 0;
					if(stack_head) stack_head->prev = new_head;
					stack_head = new_head;
					break;
				}
				// increment the current reuse distance
				if(!sentry) sentry = stack_head; // this happens when the first element was removed
				else sentry= sentry->next; 		 // Move to next element. This will be the general case
				}  // end of big while loop
			// if we reach the end we account for a cold miss
			// but if there is a hit in the last element the condition sentry==NULL will also trigger, thus we need to handle this case separately
			if(!sentry && match_bytes_pending){ 
				sentry = (struct stack_entry *) krd_malloc(sizeof(*sentry));
				if(!sentry) fprintf(log, "Malloc allocation problem\n");
				sentry->elem = element;  // create new element and push to the head of the stack
				sentry->size = size; 
				sentry->inv = 0; 
				sentry->core = core;
				sentry->next = stack_head;
				sentry->prev = NULL; 
				if(stack_head) // only if stack_head has already been initialized
					stack_head->prev = sentry;
				stack_head = sentry;
				if(!warmup_phase) 
					hist_cold_misses[threadid] += match_bytes_pending / 4; // add miss but only for pending data.
					// elements that have never been accessed before have a reuse distance of infinite. 
				}
			} // fi (!inv_probe)
			else{  // if( inv_probe )
			  // this is the path followed by invalidation probes
			  // basically go down the stack and set matching entries to invalid

				if((data_model == ARRAY_DISJOINT) || (data_model == SPARSE)){
					// if a probe accesses data that is not in the stack, then we do nothing
					if(!bloom_hash[hash_addr(element)]){ 
					sentry = NULL; // at the exit of the loop, nothing is done
					// in this case we do not need to add a new element to the entry
					}
					}
				while(sentry){
					uint64_t start;  // starting position of current match
					uint32_t bytes;  // size of current match
					visited_nodes++; // increment number of visited nodes
		
					// compute current overlap
					if(overlap(data_model, sentry->elem, sentry->size, element, size, &start, &bytes)){
						match_bytes_pending -= bytes;

						// now update the element in the trace
						if(superset(sentry, start, bytes)){ // if superset, then invalidate
							sentry->inv = 1;
							sentry->core = core; // record core that invalidatad the data
							}
						else{  // if not super, then partition and set the invalidation area depending on geometry
							repartitioned=1;
							if((start > sentry->elem) && ((start+bytes) < (sentry->elem + sentry->size))){
								// split the elements into a pre_part and a post_part and update the elements
								struct stack_entry *post_part = (struct stack_entry *) krd_malloc(sizeof(*post_part));
								struct stack_entry *inv_part = (struct stack_entry *) krd_malloc(sizeof(*post_part));
								struct stack_entry *pre_part = sentry;
								// first update values
								post_part->elem = start + bytes;
								post_part->size = (sentry->elem + sentry->size) - post_part->elem;
								post_part->inv = sentry->inv;  // copy invalid bit
								post_part->core = sentry->core;
								inv_part->elem = start;  // set invalid partition
								inv_part->size = bytes;
								inv_part->inv  = 1; 
								inv_part->core = core;
								//pre_part->elem = sentry->elem; // redundant
								//pre_part->inv = sentry->inv;	 // redundant
								//pre_part->core = sentry->core; // redundant
								pre_part->size = start-sentry->elem; // update size of invalid region
								// update next pointers
								post_part->next = sentry->next; // could be NULL
									inv_part->next = post_part;
								pre_part->next = inv_part;
								inv_part->prev = pre_part;
								post_part->prev = inv_part;
								// pre_part->prev does not change
								if(post_part->next) // update element after post_part if not the tail
									post_part->next->prev = post_part;
								}
								// The other cases are simpler
								// If the beginning matches
								else if(sentry->elem == start){
								struct stack_entry *post_part = (struct stack_entry *) krd_malloc(sizeof(*post_part));
								post_part->elem = start+bytes;
								post_part->size = sentry->size - bytes;
								post_part->inv	= sentry->inv;
								post_part->core = sentry->core;
								// modify the size of the element
								sentry->size = bytes;
								sentry->elem = start;
								sentry->inv = 1;
								sentry->core = core; // store core number
								// update pointers
								post_part->next = sentry->next;
								post_part->prev = sentry;
								sentry->next = post_part;
								// sentry->prev // stays equal
								if(post_part->next) 
									post_part->next->prev = post_part;
								}
								else{ // match is at the end
								struct stack_entry *post_part = (struct stack_entry *) krd_malloc(sizeof(*post_part));
								post_part->elem = start;
								post_part->size = bytes;
								post_part->inv = 1;
								post_part->core = core;
								sentry->size = sentry->size - bytes;
								// update pointers
								post_part->next = sentry->next;
								post_part->prev = sentry;
								sentry->next = post_part;
								//sentry->prev stays equal
								if(post_part->next) 
									post_part->next->prev = post_part;
								}
						}  // end superset check
					} // end overlap check
					if(match_bytes_pending == 0){ 
						// all bytes have been matched, just finalize
						break;
					}
				if(!sentry) sentry = stack_head; // this happens when the first element was removed
					// test special case of invalidation at the stack tail to make sure the stack does not grow unbounded
					else if(!sentry->next && sentry->prev && sentry->inv){ 
						// last entry is invalid, and need not be stored
						sentry->prev->next = NULL;
						krd_free(sentry);
						sentry=NULL;
						}  
					else sentry= sentry->next; // move to next element. This will be the general case
					}	// end of big while loop
			 } // fi (!inv_probe)

		if(partition == -1){
			if((i*100/ TTrace_num_elems(threadid)) > percent_complete){
				fprintf(log, "."); fflush(log);
				percent_complete++;
			}
		}
		else{
			if((total_bytes_processed*100 / partition) > percent_complete){
				fprintf(log, "."); fflush(log);
				percent_complete++;
			}
		}
#ifdef _DEBUG
  // check for errors in the prev links of data structure (can detect some cases of corruption)
  // Use with care, doing so each cycle results in a noticeable speed down (-> disactivated unless DEBUG)
  check_trace_consistency(i);
#endif
    if(repartitioned){
        repartitions++;
        repartitioned=0;
    }
}  // for()
  clock_gettime(CLOCK_MONOTONIC, &__tsstop); 
  __timelapse = (double)(__tsstop.tv_sec+__tsstop.tv_nsec*1e-9)-(double)(__tsstart.tv_sec+__tsstart.tv_nsec*1e-9); 
    if(i){  // if the trace is empty, don't report meaningless data
        fprintf(log, "Done!\nProcessed %d entries in %4g seconds at average rate %4g elements/sec\n", i, __timelapse, (double) i / __timelapse);
        fprintf(log, "Average size of stack: %4g elements\n", (double) visited_nodes / (double) i);
        fprintf(log, "There were %lld misses due to invalidations\n", hist_inv_misses[threadid] );
        fprintf(log, "Total number of processed bytes: %lu\n", total_bytes_processed);
        fprintf(log, "Total number of elements involving repartition: %u\n", repartitions);
        
        // the stack is fully populated at the end, so this is the time to measure the total resource usage
        getrusage(RUSAGE_SELF, &resource_usage);
        fprintf(log, "Total resident size in KiloBytes: %ld\n", resource_usage.ru_maxrss);
        print_trace_data();
    }
}


void accum_vectors(void) __attribute__((unused));
void accum_vectors(void)
{
int threadid, j;
// Now accumulate all histograms
for(threadid = 0; threadid < LOI_MAXTHREADS; threadid++)
{
    for(j = 0; j < 1<<HISTOGRAM_LEN; j++){
      hist_total_distance_vector[j] += hist_distance_vector[threadid][j];
      hist_total_shared_vector[j] += hist_shared_vector[threadid][j];
      hist_total_inv_vector[j] += hist_inv_miss_vector[threadid][j];
      total_refs +=  hist_distance_vector[threadid][j];
      }
    hist_total_cold_misses += hist_cold_misses[threadid];
    hist_total_inv_misses  += hist_inv_misses[threadid];
    total_refs +=  hist_cold_misses[threadid];
 } 
}

// write the histogram into a a specified ID
// doing so allows to generate multiple sub-histograms from the same trace
void write_one_histogram(int i, int newid)
{
   char string[30];
   int fd;

   sprintf(string, "rwi_%04d.hst",  newid);
   fd = open(string, O_CREAT | O_RDWR | O_TRUNC, S_IRWXU);

   if(write(fd, hist_cold_misses+i, sizeof(uint32_t))==-1) exit(1);
   if(write(fd, hist_inv_misses+i, sizeof(uint32_t))==-1) exit(1);
   if(write(fd, hist_distance_vector[i], (1<<HISTOGRAM_LEN)*sizeof(uint32_t))==-1) exit(1);
   if(write(fd, hist_shared_vector[i], (1<<HISTOGRAM_LEN)*sizeof(uint32_t))==-1) exit(1);
   if(write(fd, hist_inv_miss_vector[i], (1<<HISTOGRAM_LEN)*sizeof(uint32_t))==-1) exit(1);
   close(fd);
}

void read_one_histogram(int i)
{
   char string[30];
   int fd;

   sprintf(string, "rwi_%04d.hst",  i);
   fd = open(string, O_RDONLY);
   if(fd != -1){
     printf("# Reading histogram %s\n", string);
     if(read(fd, hist_cold_misses+i, sizeof(uint32_t))==-1) exit(1);
     if(read(fd, hist_inv_misses+i, sizeof(uint32_t))==-1) exit(1);
     if(read(fd, hist_distance_vector[i], (1<<HISTOGRAM_LEN)*sizeof(uint32_t))==-1) exit(1);
     if(read(fd, hist_shared_vector[i], (1<<HISTOGRAM_LEN)*sizeof(uint32_t))==-1) exit(1);
     if(read(fd, hist_inv_miss_vector[i], (1<<HISTOGRAM_LEN)*sizeof(uint32_t))==-1) exit(1);
  //   printf("Read %d elems\n", elems);
     close(fd);
   }
}

// integer power function
// http://stackoverflow.com/questions/101439/the-most-efficient-way-to-implement-an-integer-based-power-function-powint-int
// CC BY-SA 2.5 applies to this code snippet. See http://creativecommons.org/licenses/by-sa/2.5/legalcode. No changes have been made
static inline unsigned long int ipow(unsigned long int base, int exp)
{
    unsigned long int result = 1;
    while (exp)
    {
        if (exp & 1)
            result *= base;
        exp >>= 1;
        base *= base;
    }

    return result;
}

// Print out the histogram
static void print_histogram(void) __attribute__((unused));
static void print_histogram(void)
{
 printf("# Printing Histogram\n\n");

{
 int i, last=0; 
 long long int accum = 0;
 long long int accum_shared = 0;
 long long int accum_inv = 0;

 for(i = 0; i < 1<<HISTOGRAM_LEN; i++)
   if((hist_total_distance_vector[i+1] > 0) || (hist_total_distance_vector[i] > 0)) {
      accum += hist_total_distance_vector[i]; 
      accum_shared += hist_total_shared_vector[i]; 
      accum_inv += hist_total_inv_vector[i];
      fprintf(stdout, "Range %lu, Reuses %lld %g%%, Shared Reuses %lld %g%%, Cumulative Reuses %lld %g%%, Cumulative Shared Reuses %lld %g%%, "
              "Invalidation Misses %lld %g%%, Cumulative Inv %lld %g%% x x\n", ipow(2,i), 
            hist_total_distance_vector[i],100.0*hist_total_distance_vector[i]/ total_refs , 
            hist_total_shared_vector[i], 100.0*hist_total_shared_vector[i] / total_refs, accum, 100.0*accum/total_refs, accum_shared, 100.0*accum_shared/total_refs, 
            hist_total_inv_vector[i], 100.0*hist_total_inv_vector[i] / total_refs, accum_inv, 100.0*accum_inv / total_refs);
    last = i;
     }
  fprintf(stdout, "InvAndCold %lu, x x x x x x x x x x x x x x x x x x x x x x x x %lld %g%% %g%% %g%%, X X X, Cumulative %lld %g%%\n",   ipow(2,last+1) ,
            hist_total_inv_misses, 100.0*accum/total_refs,  100.0*hist_total_inv_misses/total_refs,  100.0*hist_total_cold_misses/total_refs,  accum+hist_total_inv_misses, 100.0*(accum+hist_total_inv_misses)/ total_refs);
}
 }

// This is the standalone histogram generator

#ifdef BUILD_HISTO_TOOL

int main(int argc, char *argv[])
{
  int warmup, start, newid;
  unsigned long partition; 
  unsigned int threadid; 
  int data_model = 0;  // ARRAY_PARTIAL
  int16_t kid; // kernel id

  if((argc == 4) || (argc == 3)){
	data_model = atoi(argv[1]);
	threadid = atoi(argv[2]);
    if(argc == 4)
    	kid = (int16_t) atoi(argv[3]);
    else kid = -1;
	warmup = 0; 
	start = 0;
	partition = -1;
	newid = threadid;
	}
  else if(argc == 9){
	data_model = atoi(argv[1]);
	threadid = atoi(argv[2]);
	newid  = atoi(argv[3]);
	warmup = atoi(argv[4]);
	start  = atoi(argv[5]);
	partition = atol(argv[6]);
	kid = (int16_t) atoi(argv[7]);
	}
   else { 
	printf("Usage: ./krd_tool <model> <in_trace> <out_histogram> <warmup> <histo> <partition> <kernel_id>\n");
	printf("Usage: ./krd_tool <model> <in_trace> <kernel_id>\n");
	return -1;
  }

  // Compute the histogram for a trace segment
  krd_init_index(threadid, TRACE_LENGTH*16);

  // Read the RWI trace 
  if(!krd_read_coherent_trace(threadid)) 
    return 1;

  // generate the histograms for the specified segment
  do_one_histogram_mem(data_model, threadid, warmup, start, partition, kid);

  // save the histogram just generated
  write_one_histogram(threadid, newid);

  return 0;
}

#endif // BUILD_HISTO_TOOL

#ifdef BUILD_ACCUM_TOOL

int main(int argc, char *argv[])
{
  
  //  This tool takes the histograms specified by the command line and merges them, printing
  //  a global histogram in the end
  
  if(argc == 1){
    printf("Usage ./accum_tool <sequence of histograms #IDs>\n");
        exit(1);
    }
 

  for(int i = 1; i < argc; i++){
    krd_init_index(atoi(argv[i]), TRACE_LENGTH);
    read_one_histogram(atoi(argv[i]));
    }

  accum_vectors();
  print_histogram();
  return 0;
}

#endif // BUILD_ACCUM_TOOL



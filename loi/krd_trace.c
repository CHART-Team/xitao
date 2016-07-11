/* krd_trace.c
 * Copyright 2014 Miquel Pericas and Tokyo Institute of Technology
 * This program is licensed under the terms of the New BSD License.
 * See LICENSE for full details
 *
 * krd_trace.c: store and read KRD traces
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

#define SUFFIX ".krd"

// save traces in file
// TODO: this function should use the two functions below. Actually, all three functions are almost identical and should be unified

// The function does not handle the cases of >2GB traces. However, it is rare that a single thread generates such a large trace.
// Even so, this needs to be fixed
void krd_save_traces(void)
{
  int fd, i;
  
  printf("Saving Traces...\n");

  // for each histogram we will save the values in a file for postprocessing
  for(i = 0; i < LOI_MAXTHREADS; i++){
	if(!threadtraces[i].data_ids) continue;
	if(TTrace_elem2(i,0)){ // the data elem should not be zero for this to work
		char string[20];
		sprintf(string, "raw_%04d" SUFFIX, i);
		fd = open(string, O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
		if(!fd) { 
			printf("Error: Couldn't open/create file %s for saving traces\n", string); 
			exit(1); 
			}
		printf("Writing Trace %d\n", i);
		uint32_t format= RAW_TRACE_MAGIC;
		if(write(fd, &format, sizeof(uint32_t))==-1) 
			exit(1);
#ifdef DEBUG
     printf("krd_save_traces: Writing %lu bytes to file %s\n", TTrace_num_elems(i) *sizeof(struct krd_id_tsc), string);
#endif
		if(write(fd, threadtraces[i].data_ids, TTrace_num_elems(i) *sizeof(struct krd_id_tsc)) != (TTrace_num_elems(i) *sizeof(struct krd_id_tsc))) 
			exit(1);
		close(fd);
		}
	}
   printf("Completed Saving Traces\n");
}

// save one trace by ID
void krd_write_one_trace(int i)
{
	int fd;
	ssize_t writtenbytes, total_pending = 0, offset = 0;
	// this time we do not check if the trace is actually initialized
	char string[20];
	sprintf(string, "raw_%04d" SUFFIX, i);
	fd = open(string, O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
	if(!fd) { 
			printf("Error: Couldn't open/create file %s for saving traces\n", string); 
			exit(1); 
	}
	printf("Writing Trace %d\n", i);
	uint32_t format= RAW_TRACE_MAGIC;
	if(write(fd, &format, sizeof(uint32_t))==-1) 
		exit(1);
	total_pending =  TTrace_num_elems(i) * sizeof(struct krd_id_tsc);
#ifdef DEBUG
   printf("krd_write_one_trace: Writing %lu bytes to file %s\n", total_pending, string);
#endif
   do{ 
     writtenbytes = write(fd, ((char *)(threadtraces[i].data_ids))+offset, total_pending);
     total_pending -= writtenbytes;
     offset += writtenbytes;
     } while(total_pending>0);
   close(fd);
   printf("Saved Trace %s\n", string);
}

// save one coherent trace by ID
void krd_write_coherent_trace(int i)
{
   int fd;
   ssize_t writtenbytes, total_pending = 0, offset = 0;
   // this time we do not check if the trace is actually initialized
   char string[20];
   sprintf(string, "rwi_%04d" SUFFIX, i);
   fd = open(string, O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
   if(!fd) { printf("Error: Couldn't open/create file %s for saving traces\n", string); exit(1); }
#ifdef DEBUG
   printf("Writing Trace %d\n", i);
#endif
   uint32_t format= RWI_TRACE_MAGIC;
   if(write(fd, &format, sizeof(uint32_t))==-1) exit(1);
   total_pending =  TTrace_num_elems(i) *sizeof(struct krd_id_tsc);
#ifdef DEBUG
   printf("krd_write_coherent_trace: Writing %lu bytes to file %s\n", total_pending, string);
#endif
   do{ 
     writtenbytes = write(fd, ((char *)(threadtraces[i].data_ids))+offset, total_pending);
     total_pending -= writtenbytes;
     offset += writtenbytes;
     } while(total_pending>0);
   close(fd);
   printf("Saved Trace %s\n", string);
}

// read a single thread trace into the trace memory
int krd_read_one_trace(int i)
{
  struct stat buf;
  long int elems;
  int fd; long int total_refs = 0;
  unsigned long int pending_bytes, offset=0;
  char string[20];
  int32_t header; 
  unsigned long j;

  sprintf(string, "raw_%04d" SUFFIX, i);
  fd = open(string, O_RDONLY);
  if(fd == -1){
	return 0; // this is not an error, maybe the corresponding thread did not generate a trace
	}

  fprintf(stderr, "Reading Trace %d\n", i);
  // find number of bytes 
  fstat(fd, &buf);
  pending_bytes = buf.st_size - sizeof(int32_t);

  if(read(fd, &header, sizeof(int32_t)) == -1) 
		   exit(1);
  if((header != RAW_TRACE_MAGIC) && (header != RWI_TRACE_MAGIC)){
	fprintf(stderr, "Error: unsupported magic number %u\n",header);
	exit(1);
	}
  // now, based on the magic number, update the kind of traces we are reading
  // fail if already initialized and different number
  if((krd_trace_magic >0) && (krd_trace_magic != header)){
	fprintf(stderr, "Error: using incompatible trace formats");
	exit(1);
	}

  krd_trace_magic = header;
  TTrace_num_elems(i) = pending_bytes / sizeof(struct krd_id_tsc); 

#ifdef DEBUG
  fprintf(stderr,"Recorded %lu trace elements\n", TTrace_num_elems(i));
#endif

  do{
	elems = read(fd, ((char *)(threadtraces[i].data_ids)) + offset, pending_bytes);
	if(elems == -1) { fprintf(stderr, "Error while reading trace\n"); exit(1); }
#ifdef DEBUG
       printf("krd_read_one_trace: Read total of %lu bytes\n", elems);
#endif
	pending_bytes -= elems;
	offset += elems;
	} while(pending_bytes > 0);

  for(j = 0; j < TTrace_num_elems(i); j++)
	total_refs += abs(TTrace_size2(i, j)) / sizeof(int);

  fprintf(stderr, "Trace contains total %ld LD/ST\n", total_refs);

  close(fd);
  return 1;
}

// read one coherent trace into thread trace memory
int krd_read_coherent_trace(int i)
{
	struct stat buf;
	long int elems;
	int fd; 
	unsigned long int pending_bytes, offset=0;
	long int total_refs = 0; 
	char string[20];
	long int total_invalid = 0;
	unsigned long j;
	
	sprintf(string, "rwi_%04d" SUFFIX, i);
	fd = open(string, O_RDONLY);
	if(fd == -1)
		return 0; 
	
	// find number of bytes 
	fstat(fd, &buf);
	pending_bytes = buf.st_size - sizeof(int32_t);
	
	fprintf(stderr, "Reading Trace %d\n", i);
	int32_t header; 
	if(read(fd, &header, sizeof(int32_t))==-1) exit(1);
	if((header != RWI_TRACE_MAGIC)){
		fprintf(stderr, "Error: wrong magic number %x\n", header);
		exit(1);
	}

	krd_trace_magic = header;
	TTrace_num_elems(i) = pending_bytes / sizeof(struct krd_id_tsc); 

	//unsigned long int trace_max_length = TRACE_LENGTH*sizeof(struct krd_id_tsc)*16;
	do{
		elems = read(fd, ((char *)(threadtraces[i].data_ids)) + offset, pending_bytes);
		if(elems == -1) { 
				fprintf(stderr, "Error while reading trace\n"); 
				exit(1); 
		}
#ifdef DEBUG
	fprintf(stderr, "krd_read_coherent_trace: Read total of %lu bytes\n", elems);
#endif
		pending_bytes -= elems;
		offset += elems;
	} while(pending_bytes > 0);


	for(j = 0; j < TTrace_num_elems(i); j++){
		int bytes = TTrace_size2(i, j);
		if(bytes > 0) total_refs   += bytes / sizeof(int);
		else total_invalid += abs(bytes) / sizeof(int);
	}

	fprintf(stderr, "Trace contains total %ld 4-byte loads and %ld 4-byte invalidations\n", total_refs, total_invalid);

	close(fd);
	return 1;
}


#ifdef BUILD_MERGE_TOOL

// merge several RAW traces into a single RAW trace
int main(int argc, char *argv[])
{
	int active_indexes[1024] = { 0 }; // can merge up to 1024 files
	int first_active = -1;
	unsigned long int maxelems = 0;
	unsigned long int counter=0;
	uint64_t lowest_tsc = 0;
	int source_tid, target_tid; 
	long target_index, source_index;
	int tid;

	if (argc == 1) {
		printf("Usage: ./merge_tool <output id>  <trace ids> \n");
		exit(1);
	}

  // Note that this first version will override an existing trace
   
  // Initialize target TID
	target_tid = atoi(argv[1]);
	krd_init_index(target_tid, TRACE_LENGTH*16);

  // Read all the files
	for(int i = 2; i < argc; i++){
		krd_init_index(atoi(argv[i]), TRACE_LENGTH);
		if(krd_read_one_trace(atoi(argv[i]))){
			active_indexes[i] = 1;
			if(first_active < 0) first_active = i;
		}
	  }


  // Now create a time merging into the output trace
	for(int i = 2; i < argc; i++)
		if(active_indexes[i])
			maxelems += threadtraces[atoi(argv[i])].num_elems; //TTrace_num_elems();

  // maxelems is now the size of the longest trace
	printf("maxelems set to %lu\n", maxelems);

	while(counter < maxelems){
		// initialize lowest_tsc and source_tid values
		// needs to be done for each iteration
		lowest_tsc = -1; //TTrace_next_tsc(atoi(argv[first_active]));
		source_tid = atoi(argv[first_active]);

		// which data block has the lowest tsc? 
		for(int i = first_active; i < argc; i++)
			if(active_indexes[i]){
				tid = atoi(argv[i]);
				// a TSC of value 0 indicates that this element is beyond the trace's end
				// In this case we just jump to and try the next element
				if(threadtraces[tid].data_ids[threadtraces[tid].index].tsc == 0) continue;

				if(threadtraces[tid].data_ids[threadtraces[tid].index].tsc < lowest_tsc){
					source_tid = tid;
					lowest_tsc = threadtraces[tid].data_ids[threadtraces[tid].index].tsc;
				}
			}

		// now we simply copy the information
		target_index = threadtraces[target_tid].index;
		source_index = threadtraces[source_tid].index;
		
		threadtraces[target_tid].data_ids[target_index].tsc = threadtraces[source_tid].data_ids[source_index].tsc;
		threadtraces[target_tid].data_ids[target_index].id = threadtraces[source_tid].data_ids[source_index].id;
		threadtraces[target_tid].data_ids[target_index].size = threadtraces[source_tid].data_ids[source_index].size;
		threadtraces[target_tid].data_ids[target_index].kernel_id = threadtraces[source_tid].data_ids[source_index].kernel_id;
		threadtraces[target_tid].data_ids[target_index].core = source_tid; // threadtraces[source_tid].data_ids[source_index].core;
	
		// NOTE: here we are using the trace id as the source core number. 
		// Thus, when merging already "merged" traces, the recorded ID will be that of the merged trace, not the source 
		// core. To keep this information, only a single merge should be done before generation of coherent traces
		// However, this has the advantage that we can easily analyze inter-socket sharing by meging all the socket traces first, 
		// and then generating a globally merged trace
		// Note that the coherent trace generation does not use this method, thus the original core is kept in that case
		
		// in that case we just need to copy it here
		// But need to check that these copies have negligible impact
		//threadtraces[target_tid].data_ids[target_index].numa = 0;  
		threadtraces[target_tid].index++;
		threadtraces[source_tid].index++;
		
		counter++;
		}

	target_tid = atoi(argv[1]);
	threadtraces[target_tid].num_elems = counter;
	printf("Target trace has %lu elements\n", counter);
	
	krd_write_one_trace(target_tid);
	return 0;
}

#endif // BUILD_MERGE_TOOL

#ifdef BUILD_COHERENCE_TOOL

// this is the "coherent" trace generation tool
// Format ./tool <#rwi_trace> <raw_trace> <#raw_in1> <#raw_in2> ... <#raw_inN>
// The tool selects a raw trace for which to generate a coherent trace <raw_trace> and a set of 
// conflicting traces raw_in1..raw_inN that will generate invalidations into the file
// The rwi trace is generated with its own identifier

// The operation is simple. All raw traces are opened. The elements of the raw_trace are converted to 
// regular accesses in the rwi trace. The writes of the raw_in traces are copied as invalidations into 
// the rwi_trace. Reads in the raw traces are ignored.

int main(int argc, char *argv[])
{
	int active_indexes[1024] = { 0 }; // can merge up to 1024 files
	int first_active = -1;
	unsigned long int maxelems = 0;
	unsigned long int counter=0;
	uint64_t lowest_tsc = 0;
	int source_tid = 0;
	int out_tid;
	int target_tid;
	 
	int tid;
	
	if (argc == 1) {
		printf("Usage: ./coherence_tool <output id>  <input id>  <confl. ids> \n");
		exit(1);
		}
	
	// Initialize target TID
	target_tid = atoi(argv[1]);
	krd_init_index(target_tid, TRACE_LENGTH*16); // output slightly larger
	
	out_tid = atoi(argv[2]);
	
	// Read all the files
	for(int i = 2; i < argc; i++){
		krd_init_index(atoi(argv[i]), TRACE_LENGTH*16);
		if(krd_read_one_trace(atoi(argv[i]))){
			active_indexes[atoi(argv[i])] = 1;
			if(first_active < 0) first_active = i;  // should always be 2
			}
	}
	
	// After this all the traces have been read.
	// Now we start merging, but it is different whether we are reading from first_active or from others
	
	// Now create a time merging into the output trace
	// Initially we just count the total number of elements
	
	for(int i = 2; i < argc; i++){
		tid = atoi(argv[i]);
		if(active_indexes[tid])
			maxelems += threadtraces[tid].num_elems;
		}
	
	printf("maxelems is %lu\n", maxelems);
	
	// maxelems is now the size of all elements together. 
	// It is not the size of the coherent output trace, which will be determined at runtime, 
	//    and which depends on the number of writes in the raw traces
	
	// counter will be increased for each element that is processed, even though the counter 
	// of the output trace will only be increased for each recorded element
	while(counter < maxelems){
		// initialize lowest_tsc and source_tid values
		// needs to be done for each iteration
		lowest_tsc = -1;  // since this is unsigned, this translates to the largest possible value
	
		// which data block has the lowest tsc? 
		for(int i = first_active; i < argc; i++){
			tid = atoi(argv[i]);
			if(active_indexes[tid]){
				int index = threadtraces[tid].index;
				if(threadtraces[tid].data_ids[index].tsc == 0) continue;
			if(threadtraces[tid].data_ids[index].tsc < lowest_tsc){
				source_tid = tid;
				lowest_tsc = threadtraces[tid].data_ids[threadtraces[tid].index].tsc;
				} // if
			} // if
			} // for
		// at this point source_tid contains the next thread with lowest TSC
		// now there are two paths
		// if the thread is the same as out_thread then just copy the information, but keep the task size positive
		// otherwise check if this is a write. If it is a read, just increment the counter but do nothing else
		//  if it is a write, then include a write with negative number of bytes (interpreted as invalidation)
		
		// if this entry belongs to the output trace. Add all elements as normal accesses
		// This one stores the original core id, not the identifier of the tracefile
		if(source_tid == out_tid){
			int target_index = threadtraces[target_tid].index;
			int source_index = threadtraces[source_tid].index;
			threadtraces[target_tid].data_ids[target_index].tsc = threadtraces[source_tid].data_ids[source_index].tsc;
			threadtraces[target_tid].data_ids[target_index].id  = threadtraces[source_tid].data_ids[source_index].id;
			threadtraces[target_tid].data_ids[target_index].size  = abs(threadtraces[source_tid].data_ids[source_index].size);
			threadtraces[target_tid].data_ids[target_index].kernel_id  = threadtraces[source_tid].data_ids[source_index].kernel_id;
			threadtraces[target_tid].data_ids[target_index].core  = threadtraces[source_tid].data_ids[source_index].core;
			threadtraces[target_tid].index++;
		}
		else if(threadtraces[source_tid].data_ids[threadtraces[source_tid].index].size < 0){   // add an invalidation
			int target_index = threadtraces[target_tid].index;
			int source_index = threadtraces[source_tid].index;
			threadtraces[target_tid].data_ids[target_index].tsc = threadtraces[source_tid].data_ids[source_index].tsc;
			threadtraces[target_tid].data_ids[target_index].id  = threadtraces[source_tid].data_ids[source_index].id;
			threadtraces[target_tid].data_ids[target_index].size  = threadtraces[source_tid].data_ids[source_index].size;
			threadtraces[target_tid].data_ids[target_index].kernel_id  = threadtraces[source_tid].data_ids[source_index].kernel_id;
			threadtraces[target_tid].data_ids[target_index].core  = threadtraces[source_tid].data_ids[source_index].core;
			threadtraces[target_tid].index++;
		}
		threadtraces[source_tid].index++;
		if(threadtraces[source_tid].index >= threadtraces[source_tid].num_elems) 
			active_indexes[source_tid] = 0; // deactivate if all elements have been processed
		counter++;
	 }
	// record the final number of elements that have been processed
	threadtraces[target_tid].num_elems = threadtraces[target_tid].index;
	printf("Recorded final number of elements %lu, counter = %lu\n", threadtraces[target_tid].num_elems, counter);
	printf("Writing coherent trace file\n");
	
	krd_write_coherent_trace(atoi(argv[1]));
	return 0;
}

#endif // BUILD_COHERENT_TOOL

#ifdef BUILD_INTERLEAVE_TOOL

int main(int argc, char *argv[])
{
	unsigned int maxelems = 0;
	unsigned int counter, numThreads;
	
	if (argc == 1) {
		printf("Usage: ./intlv_tool <output id>  <trace ids> \n");
		exit(1);
		}
	
	// Note that this first version will override an existing trace
	
	// output trace needs to be larger
	krd_init_index(atoi(argv[1]), TRACE_LENGTH*16);
	 
	for(int i = 2; i < argc; i++){
		krd_init_index(atoi(argv[i]), TRACE_LENGTH);
		krd_read_one_trace(atoi(argv[i]));
		}
	
	// Now create an interleaving into the output trace
	numThreads = argc - 2;
	for(int i = 2; i < argc; i++)
		if(TTrace_num_elems(atoi(argv[i])) > maxelems)
			maxelems = TTrace_num_elems(atoi(argv[i]));
	
	for(counter = 0; (counter < maxelems*numThreads) && (counter < TRACE_LENGTH); ){
		for(int i = 2; i < argc; i++){
		if(counter/numThreads > TTrace_num_elems(atoi(argv[i])) ) {  
				counter++; 
				continue;
				}
		TTrace_elem2(atoi(argv[1]), counter) = TTrace_elem2(atoi(argv[i]), counter/numThreads);
		counter++;
		}
	}
	
	TTrace_num_elems(atoi(argv[1])) = counter;
	
	krd_write_one_trace(atoi(argv[1]));
	return 0;
}

#endif // BUILD_INTERLEAVE_TOOL

#ifdef BUILD_DUMP_TOOL

int main(int argc, char *argv[])
{
	// This tool reads a trace and dumps it to the output
	int index; 
	
	if(argc != 3){
		printf("Usage ./dump_tool <trace> <nelems>\n");
		exit(1);
	}
	
	index = atoi(argv[1]);
	krd_init_index(index, TRACE_LENGTH*16);
	krd_read_one_trace(index);
	krd_read_coherent_trace(index);
	
	printf("This trace is a %s trace\n", (krd_trace_magic == RAW_TRACE_MAGIC)? "RAW" : "RWI");
	
	for(int i = 0; i < atoi(argv[2]); i++){
		printf("Element Address %lu, Element Size %d, Timestamp %lu, Core %d, Kernel ID %d\n", 
		TTrace_elem2(index, i), TTrace_size2(index, i), TTrace_tsc2(index, i), TTrace_core2(index,i), TTrace_id2(index, i));
		}
	
	return 0;
}
#endif // BUILD_DUMP_TOOL

#ifdef BUILD_CUT_TOOL

// This tool reads a trace and sets partition ranges for parallelization
int main(int argc, char *argv[])
{
	int index, psize, wsize;
	int w, p;
	long warmup, partition; 
	long w_p, p_p; // warmup point and partition point (counted in bytes)
	int count = 0; // number of partitions that have been printed
	int index_offset; 
	int data_model = 0;
	
	if(argc != 6){
		printf("Usage: partition_tool <data_model> <trace> <index_offset> <warm-up> <partition-size>\n");
			exit(1);
			}

	data_model = atoi(argv[1]);
	index = atoi(argv[2]);
	index_offset = atoi(argv[3]);
	warmup = atol(argv[4]);
	partition = atol(argv[5]);
	
	krd_init_index(index, TRACE_LENGTH*16);
	krd_read_coherent_trace(index);
	
	// set current points to zero
	w_p = 0;
	p_p = 0;
	p = w = -1;
	
	// the first range is always at 0 
	printf("#!/bin/bash\n");
	//printf("%s %d %d 0 0 %ld &\n", cmd[data_model], index, index_offset, partition);
	printf("krd_tool %d %d %d 0 0 %ld -1 \n", data_model, index, index_offset, partition);
	
	// advance pointers so that they point to the first valid entry
	do{
		p++;
		psize = threadtraces[index].data_ids[p].size;
		}while(psize < 0);
	  
	do{
		w++;
		wsize = threadtraces[index].data_ids[w].size;
		}while(wsize < 0);
	
	// now loop through the trace trying to find partition points
	while(p < (int) threadtraces[index].num_elems){
	
		// increment the current p_p
		p_p += psize;
		
		// if the distance of p_p and w_p is more than the warm-up range, then increment w_p and advance pointers
		if((p_p - w_p) > warmup){
			 w_p += wsize;
			// advance to next w-element that is not an invalidation
			do{
				w++;
				wsize = threadtraces[index].data_ids[w].size;
				}while(wsize < 0);
			}
		
		// if p_p has reached a partition point, then print out the ranges
		if((p_p / partition) > count){
			//  printf("DEBUG: p_p %ld, partition %ld, count %d\n", p_p, partition, count);
			//printf("%s %d %d %d %d %ld &\n", cmd[data_model], index, index_offset + count + 1, w, p, partition);
			printf("krd_tool %d %d %d %d %d %ld -1 \n", data_model, index, index_offset + count + 1, w, p, partition);
			count++;
			}
		
		// advance to next p-element that is not an invalidation
		do{
			p++;
			psize = threadtraces[index].data_ids[p].size;
			}while(psize < 0);
		
	}

	printf("wait\n");
	printf("accum_tool `seq %d %d`\n", index_offset, index_offset + count);
	return 0;
}

#endif // BUILD_CUT_TOOL

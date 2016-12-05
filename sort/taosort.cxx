// taosort.cxx
//

#include "taosort.h"

#ifdef DO_LOI
#include "loi.h"
#endif

int worker_loop(int);

int sort_verify ( void )
{
     int i, success = 1;
     for (i = 0; i < sort_buffer_size; ++i)
      if (array[i] != i) success = 0;

      return success ;
}

void print_array( int a, ELM *arr )
{
        if(a < 1) return;
        if(a > 100) a = 100;
        for(int i =0; i < a; i++) 
                printf("Elem %d = %ld\n", i, arr[i]);
}

#define BLOCKSIZE (2*1024)

int main ( int argc, char *argv[] )
{
        PolyTask *st1;
        int i,j,k;

        int blocksize;
        int qsort_width;

	int thread_base, nthreads;

   // set the number of threads and thread_base

   if(getenv("GOTAO_NTHREADS"))
        nthreads = atoi(getenv("GOTAO_NTHREADS"));
   else 
        nthreads = GOTAO_NTHREADS;

   if(getenv("GOTAO_THREAD_BASE"))
        thread_base = atoi(getenv("GOTAO_THREAD_BASE"));
   else
        thread_base = GOTAO_THREAD_BASE;

// just five levels of sorting
// do not attempt generic depth for now
	gotao_init(nthreads,thread_base);

#if LEVEL1 == 1
#define TAOQuickMerge TAOQuickMergeDyn
        TAOQuickMergeDyn *level1[256];
#elif LEVEL1 == 2
#define TAOQuickMerge TAOQuickMerge2
        TAOQuickMerge2 *level1[256];
#elif LEVEL1 == 4
#define TAOQuickMerge TAOQuickMerge4
        TAOQuickMerge4 *level1[256];
#endif 

#if LEVEL2 == 1
#define TAOMerge_2 TAOMergeDyn
        TAOMergeDyn *level2[64];
#elif LEVEL2 == 2
#define TAOMerge_2 TAOMerge2
        TAOMerge2 *level2[64];
#elif LEVEL2 == 4
#define TAOMerge_2 TAOMerge4
        TAOMerge4 *level2[64];
#endif 

#if LEVEL3 == 1
#define TAOMerge_3 TAOMergeDyn
        TAOMergeDyn *level3[16];
#elif LEVEL3 == 2
#define TAOMerge_3 TAOMerge2
        TAOMerge2 *level3[16];
#elif LEVEL3 == 4
#define TAOMerge_3 TAOMerge4
        TAOMerge4 *level3[16];
#endif 



#if LEVEL4 == 1
#define TAOMerge_4 TAOMergeDyn
        TAOMergeDyn *level4[4];
#elif LEVEL4 == 2
#define TAOMerge_4 TAOMerge2
        TAOMerge2 *level4[4];
#elif LEVEL4 == 4
#define TAOMerge_4 TAOMerge4
        TAOMerge4 *level4[4];
#endif 


#if LEVEL5 == 1
#define TAOMerge_5 TAOMergeDyn
        TAOMergeDyn *level5[1];
#elif LEVEL5 == 2
#define TAOMerge_5 TAOMerge2
        TAOMerge2 *level5[1];
#elif LEVEL5 == 4
#define TAOMerge_5 TAOMerge4
        TAOMerge4 *level5[1];
#endif 

        sort_buffer_size = 16384*BLOCKSIZE;
        insertion_thr    = 20;

        array = (ELM *) malloc(sort_buffer_size * sizeof(ELM));
        tmp = (ELM *) malloc(sort_buffer_size * sizeof(ELM));

        fill_array();
        scramble_array();

        std::chrono::time_point<std::chrono::system_clock> start, end;
        std::cout << "Sorting " << sort_buffer_size << " integers via 4-1 TAOSort " << std::endl;
  	start = std::chrono::system_clock::now();

	for(int i = 0; i < 256; i++){
                level1[i] = new TAOQuickMerge(array + 64*i*BLOCKSIZE, tmp + 64*i*BLOCKSIZE, 64*BLOCKSIZE, DYNW1);
                st1 = (PolyTask *) level1[i];
#if defined(PLACEMENT_DISTRIBUTED) && defined(TAO_PLACES)
                st1->set_place( ((float) i/ (float) 256));
#endif
                gotao_push(st1);
	}

        for(i = 0; i < 64; i++){
                level2[i] = new TAOMerge_2(array + 256*i*BLOCKSIZE, tmp + 256*i*BLOCKSIZE, 256*BLOCKSIZE, DYNW2);
                for(j = i*4; j < (i+1)*4; j++)
                        level1[j]->make_edge(level2[i]);
        }

        for(i = 0; i < 16; i++){
                level3[i] = new TAOMerge_3(array + 1024*i*BLOCKSIZE, tmp + 1024*i*BLOCKSIZE, 1024*BLOCKSIZE, DYNW3);
                for(j = i*4; j < (i+1)*4; j++)
                        level2[j]->make_edge(level3[i]);
        }

        for(i = 0; i < 4; i++){
                level4[i] = new TAOMerge_4(array + 4096*i*BLOCKSIZE, tmp + 4096*i*BLOCKSIZE, 4096*BLOCKSIZE, DYNW4);
                for(j = i*4; j < (i+1)*4; j++)
                        level3[j]->make_edge(level4[i]);
        }

        for(i = 0; i < 1; i++){
                level5[i] = new TAOMerge_5(array + 16384*i*BLOCKSIZE, tmp + 16384*i*BLOCKSIZE, 16384*BLOCKSIZE, DYNW5);
                for(j = i*4; j < (i+1)*4; j++)
                        level4[j]->make_edge(level5[i]);
        }


// LOI instrumentation
#if DO_LOI
    loi_init(); // calc TSC freq and init data structures
    printf(" TSC frequency has been measured to be: %g Hz\n", (double) TSCFREQ);
    int maxthr = nthreads;
#endif

      
#ifdef EXTRAE
        std::cout << "Total number of tasks is " <<  PolyTask::created_tasks << "\n";
#endif

   	goTAO_start();
#ifdef DO_LOI 
        phase_profile_start();
#endif

  	goTAO_fini();
      
#ifdef DO_LOI
        phase_profile_stop(0); 
#endif

  	end = std::chrono::system_clock::now();
        std::chrono::duration<double> elapsed_seconds = end-start;
        std::time_t end_time = std::chrono::system_clock::to_time_t(end);
      
        std::cout << "elapsed time: " << elapsed_seconds.count() << "s. " << "Total number of steals: " <<  tao_total_steals << "\n";

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


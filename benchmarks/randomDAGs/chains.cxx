/*
	Chains benchmark
	creates a DAG consisting of parallel chains of TAOs. 
  the lenght of the chains and parallelism can be adjusted in config-chains-bench.h

*/
#include "tao.h"
#include "taomatrix.h"
#include "taosort.h"
#include "taocopy.h"
#include <chrono>
#include <iostream>
#include <atomic>
#include "config-chains-bench.h"



extern "C" {

#include <stdio.h>
#include <stdlib.h> 
#include <unistd.h>

}

//used for the size of the sort kernel
#define BLOCKSIZE (2*1024)

void fill_arrays(int **a, int **c, int ysize, int xsize);
#if defined(CRIT_PERF_SCHED)
extern int TABLEWIDTH;
float TAO_matrix::time_table[XITAO_MAXTHREADS][XITAO_MAXTHREADS];
float TAOQuickMergeDyn::time_table[XITAO_MAXTHREADS][XITAO_MAXTHREADS];
float TAO_Copy::time_table[XITAO_MAXTHREADS][XITAO_MAXTHREADS];
#endif


// MAIN 
int
main(int argc, char* argv[])
{

  int matrix_width; //number of chains consisting of matrix multiolication TAOs
  int sort_width; // number of chains consisting of sortTAOs
  int heat_width; //number of chains consisting of copyTAOs
  int dag_depth; //the length of all chain
  int sort_size; //working set size of sort
  int heat_resolution; //working set size of copy
  int ma_width; //matrix assembly width
  int sa_width; //sort assebly width
  int ha_width; //copy assembly width

   int thread_base; int nthreads; int nctx; 
   int r_size;
   int nqueues;  
   int nas;      

 // set the number of threads and thread_base

   if(getenv("XITAO_MAXTHREADS"))
        nthreads = atoi(getenv("XITAO_MAXTHREADS"));
   else
        nthreads = XITAO_MAXTHREADS;

   if(getenv("GOTAO_THREAD_BASE"))
        thread_base = atoi(getenv("GOTAO_THREAD_BASE"));
   else
        thread_base = GOTAO_THREAD_BASE;

   if(getenv("GOTAO_HW_CONTEXTS"))
        nctx = atoi(getenv("GOTAO_HW_CONTEXTS"));
   else
        nctx = GOTAO_HW_CONTEXTS;

   if(getenv("ROW_SIZE"))
	r_size=atoi(getenv("ROW_SIZE"));
 
   else
	r_size=ROW_SIZE;

   if(getenv("DAG_DEPTH"))
  dag_depth=atoi(getenv("DAG_DEPTH"));
   else
  dag_depth=DAG_DEPTH;

   if(getenv("MATRIX_WIDTH"))
  matrix_width=atoi(getenv("MATRIX_WIDTH"));
   else
  matrix_width=MATRIX_WIDTH;

  if(getenv("SORT_WIDTH"))
  sort_width=atoi(getenv("SORT_WIDTH"));
   else
  sort_width=SORT_WIDTH;

  if(getenv("SORT_SIZE"))
  sort_size=atoi(getenv("SORT_SIZE"));
   else
  sort_size=SORT_SIZE;

  if(getenv("HEAT_WIDTH"))
  heat_width=atoi(getenv("HEAT_WIDTH"));
   else
  heat_width=HEAT_WIDTH;

  if(getenv("HEAT_RESOLUTION"))
  heat_resolution=atoi(getenv("HEAT_RESOLUTION"));
   else
  heat_resolution=HEAT_RESOLUTION;

  if(getenv("M_ASSEMBLY_WIDTH"))
  ma_width=atoi(getenv("M_ASSEMBLY_WIDTH"));
   else
  ma_width=M_ASSEMBLY_WIDTH;

  if(getenv("S_ASSEMBLY_WIDTH"))
  sa_width=atoi(getenv("S_ASSEMBLY_WIDTH"));
   else
  sa_width=S_ASSEMBLY_WIDTH;

  if(getenv("H_ASSEMBLY_WIDTH"))
  ha_width=atoi(getenv("H_ASSEMBLY_WIDTH"));
   else
  ha_width=H_ASSEMBLY_WIDTH;




//memory allocation

  if (matrix_width == 0){
    r_size = 0;
  }
  int m_ysize = r_size;

  int m_xsize = (r_size * matrix_width * 2);

int** matrix_input_a = new int* [m_ysize];
int** matrix_output_c = new int* [m_ysize];

for (int i = 0; i < m_ysize; ++i)
{
  matrix_input_a[i] = new int[m_xsize];
  matrix_output_c[i] = new int[m_xsize];
}



  int s_ysize = sort_width;
  int s_xsize = sort_size*BLOCKSIZE;

int** sort_input_a = new int* [s_ysize];
int** sort_output_c = new int* [s_ysize];

for (int i = 0; i < s_ysize; ++i)
{
  sort_input_a[i] = new int[s_xsize];
  sort_output_c[i] = new int[s_xsize];
}



  int h_ysize = heat_width+2;
  int h_xsize = heat_resolution*heat_resolution;


int** heat_input_a = new int* [h_ysize];
int** heat_output_c = new int* [h_ysize];

for (int i = 0; i < h_ysize; ++i)
{
  heat_input_a[i] = new int[h_xsize];
  heat_output_c[i] = new int[h_xsize];
}





    std::cout << "gotao_init parameters are: " << nthreads <<"," << thread_base << ","<< nctx << std::endl;

   gotao_init();








   // fill the input arrays and empty the output array
   fill_arrays(matrix_input_a , matrix_output_c, m_ysize, m_xsize);
      fill_arrays(sort_input_a , sort_output_c, s_ysize, s_xsize);
         fill_arrays(heat_input_a , heat_output_c, h_ysize, h_xsize);

   //Spawn TAOs for every seperately written too value, equivilent to for(i->i+stepsize){for(j->j+stepsize) generation}
   int matrix_assemblies = dag_depth * matrix_width;
   TAO_matrix *matrix_ao[matrix_assemblies];
   int sort_assemblies = dag_depth * sort_width;
   TAOQuickMergeDyn *sort_ao[sort_assemblies];
   int heat_assemblies = dag_depth * heat_width;
   TAO_Copy *heat_ao[heat_assemblies];



   //infices to keep track of spawned TAOs
  int i = 0;
  int j = 0;
  int k = 0;

//  int** input;
  //int** output;
  for (int x = 0; x < dag_depth; x++)
  {
    // alternate input and output between steps of the DAG to make heat and matrix mult data-dependent
     
     //spawn Matrix multiplication taos
    for (int y = 0; y < matrix_width; y++)
    {
      matrix_ao[i] = new TAO_matrix(ma_width, //taowidth
                                    0, //start y
                                    r_size, //stop y
                                    y*r_size*2, //start x
                                    r_size+y*r_size*2, //stop x
                                    0, //output offset (if needed)
                                    r_size, 
                                    matrix_input_a,
                                    matrix_output_c);
      if (x == 0) {
        gotao_push(matrix_ao[i], i % nthreads);
      } else {
        matrix_ao[i-matrix_width]->make_edge(matrix_ao[i]);
      }
      i++;
    }

    //spawn sort TAOs
    for (int z = 0; z < sort_width; z++)
    {
      sort_ao[j] = new TAOQuickMergeDyn(sort_input_a[z],
                                        sort_output_c[z], 
                                        sort_size*BLOCKSIZE,
                                        sa_width);
      if (x == 0) {
        gotao_push(sort_ao[j], j % nthreads);
      } else {
        sort_ao[j-sort_width]->make_edge(sort_ao[j]);
      }
      j++;
    }

    //spawn copy TAOs
    for (int z = 0; z < heat_width; z++)
    {
      heat_ao[k] = new TAO_Copy(
                             heat_input_a[z+1],
                             heat_output_c[z+1],
                             heat_resolution * heat_resolution,
                             ha_width);
      if (x == 0) {
        gotao_push(heat_ao[k], k % nthreads);
      } else {
        heat_ao[k-heat_width]->make_edge(heat_ao[k]);
      }
      k++;
    }

  }
      




   // measure execution time
std::cout << "starting \n";
   std::chrono::time_point<std::chrono::system_clock> start, end;
   start = std::chrono::system_clock::now();

   goTAO_start();

   goTAO_fini();

   end = std::chrono::system_clock::now();

   std::chrono::duration<double> elapsed_seconds = end-start;
   std::time_t end_time = std::chrono::system_clock::to_time_t(end);
 
   std::cout << "elapsed time: " << elapsed_seconds.count() << "s. " << "Total number of steals: " <<  tao_total_steals << "\n";
   std::cout << "Assembly Throughput: " << (matrix_assemblies + heat_assemblies + sort_assemblies) / elapsed_seconds.count() << " A/sec\n";
   std::cout << "Assembly Cycle: " << elapsed_seconds.count() / (matrix_assemblies + heat_assemblies + sort_assemblies)  << " sec/A\n";
#ifdef CRIT_PERF_SCHED
   for(int threads =0; threads< XITAO_MAXTHREADS; threads++){
	   std::cout <<"Tao Matrix: \n";
	   for (int count=0; count < TABLEWIDTH; count++){
   		std::cout << "Time table content for core " << threads  <<": " << TAO_matrix::time_table[threads][count] << "\n";
   	   }
   }
   for(int threads =0; threads< XITAO_MAXTHREADS; threads++){
	   std::cout <<"Tao Sort: \n";
	   for (int count=0; count < TABLEWIDTH; count++){
   		std::cout << "Time table content for core " << threads  <<": " << TAOQuickMergeDyn::time_table[threads][count] << "\n";
   	   }
   }
   for(int threads =0; threads< XITAO_MAXTHREADS; threads++){
	   std::cout <<"Tao Copy: \n";
	   for (int count=0; count < TABLEWIDTH; count++){
   		std::cout << "Time table content for core " << threads  <<": " << TAO_Copy::time_table[threads][count] << "\n";
   	   }
   }

#endif
  for (int i = 0; i < m_ysize; ++i)
{
  delete matrix_input_a[i];
  //delete matrix_input_b[i];
  delete matrix_output_c[i];
}
delete[] matrix_input_a;
//delete[] matrix_input_b;
delete[] matrix_output_c;


  for (int i = 0; i < s_ysize; ++i)
{
  delete sort_input_a[i];
  //delete matrix_input_b[i];
  delete sort_output_c[i];
}
delete[] sort_input_a;
//delete[] matrix_input_b;
delete[] sort_output_c;


  for (int i = 0; i < h_ysize; ++i)
{
  delete heat_input_a[i];
  //delete matrix_input_b[i];
  delete heat_output_c[i];
}
delete[] heat_input_a;
//delete[] matrix_input_b;
delete[] heat_output_c;


   return (0);
}

void fill_arrays(int **a, int **c, int ysize, int xsize)
{

     //Fills the input matrices a and b with random integers and output matrix with 0
     for (int i = 0; i < ysize; ++i) { //each row
        for (int j = 0; j  < ysize; j++){ //each column
          a[i][j] = (rand() % 111);
          c[i][j] = (rand() % 111);
        }
     }
}


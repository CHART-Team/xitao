//
//
//

#include "../tao.h"
#include "taomatrix.h"
#include "solver-tao.h"
#include "taosort.h"
#include <chrono>
#include <iostream>
#include <atomic>
#include "config-static-bench.h"



extern "C" {

#include <stdio.h>
#include <stdlib.h> 
#include <unistd.h>

}


#define BLOCKSIZE (2*1024)


void fill_arrays(int **a, int **b, int **c, int ysize, int xsize);


// MAIN 
int
main(int argc, char* argv[])
{

  int matrix_width;
  int sort_width;
  int heat_width;
  int dag_depth;
  int dag_width;
  int sort_size;
  int heat_resolution;
  int xdecomp;
  int ydecomp;
  int ma_width; //matrix assembly width
  int sa_width;
  int ha_width;

   int thread_base; int nthreads; int nctx; //Do we need the last parameter?
   int c_size; int r_size; int stepsize;
   int nqueues;  // how many queues to fill: What is this??
   int nas;      // number of assemblies per queue: what is this??


  


 // set the number of threads and thread_base

   if(getenv("GOTAO_NTHREADS"))
        nthreads = atoi(getenv("GOTAO_NTHREADS"));
   else
        nthreads = GOTAO_NTHREADS;

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

   if(getenv("COL_SIZE"))
	c_size=atoi(getenv("COL_SIZE"));
   else
	c_size=COL_SIZE;

   if(getenv("STEP_SIZE"))
	stepsize=atoi(getenv("STEP_SIZE"));
   else
	stepsize=STEP_SIZE;

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

  if(getenv("INTERNAL_XDECOMP"))
  xdecomp=atoi(getenv("INTERNAL_XDECOMP"));
   else
  xdecomp=INTERNAL_XDECOMP;

  if(getenv("INTERNAL_YDECOMP"))
  ydecomp=atoi(getenv("INTERNAL_YDECOMP"));
   else
  ydecomp=INTERNAL_YDECOMP;

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

   if((COL_SIZE != ROW_SIZE) || (0!=(COL_SIZE % stepsize))){
	std::cout << "Incompatible matrix parameters, please choose ROW_SIZE=COL_SIZE and a stepsize that is a divisior of the ROW_SIZE&&COL_SIZE";
	return 0;
   }

   #define ceildiv(a,b) ((a + b -1)/(b))


  dag_width = matrix_width + sort_width + heat_width;



  //TAOSORT inits
  sort_buffer_size = sort_size*BLOCKSIZE;
  insertion_thr    = 20;

  int inputsize = std::max(heat_resolution, ROW_SIZE);
  int ysize = sort_width*2 + heat_resolution+2 + ROW_SIZE;
  int xsize = std::max(std::max((heat_resolution*heat_resolution+2), (sort_size*BLOCKSIZE)), (ROW_SIZE * matrix_width));


int** matrix_input_a = new int* [ysize];
int** matrix_input_b = new int* [ysize];
int** matrix_output_c = new int* [ysize];

for (int i = 0; i < ysize; ++i)
{
  matrix_input_a[i] = new int[xsize];
  matrix_input_b[i] = new int[xsize];
  matrix_output_c[i] = new int[xsize];
}




/*
  int **matrix_input_a = (int**) malloc(sizeof *matrix_input_a * xsize);
  int **matrix_input_b = (int**) malloc(sizeof *matrix_input_a * xsize);
  int **matrix_output_c = (int**) malloc(sizeof *matrix_output_c * xsize);

  

  if (matrix_input_a && matrix_input_b && matrix_output_c){
    for (int i = 0; i < xsize; i++){
      matrix_input_a[i] = (int*) malloc(sizeof *matrix_input_a[i] * ysize);
      matrix_input_b[i] = (int*) malloc(sizeof *matrix_input_a[i] * ysize);
      matrix_output_c[i] = (int*) malloc(sizeof *matrix_output_c[i] * ysize); 
    }
  }
*/



  array = (ELM *) malloc(sort_buffer_size * sizeof(ELM));
  tmp = (ELM *) malloc(sort_buffer_size * sizeof(ELM));

        fill_array();
        scramble_array();



    std::cout << "gotao_init parameters are: " << nthreads <<"," << thread_base << ","<< nctx << std::endl;

   gotao_init();


   std::chrono::time_point<std::chrono::system_clock> start, end;
   start = std::chrono::system_clock::now();



/*
           TAOinit *inits[256];
        sort_buffer_size = 16384*2048;
        insertion_thr    = 20;

        array1 = (ELM *) malloc(sort_buffer_size * sizeof(ELM));
        tmp = (ELM *) malloc(sort_buffer_size * sizeof(ELM));
*/



   // fill the input arrays and empty the output array
   fill_arrays(matrix_input_a, matrix_input_b , matrix_output_c, ysize, xsize);

   //Spawn TAOs for every seperately written too value, equivilent to for(i->i+stepsize){for(j->j+stepsize) generation}
   int matrix_assemblies = dag_depth * matrix_width;
   TAO_matrix *matrix_ao[matrix_assemblies];
   int sort_assemblies = dag_depth * sort_width;
   TAOQuickMergeDyn *sort_ao[sort_assemblies];
   int heat_assemblies = dag_depth * heat_width;
   jacobi2D *heat_ao[heat_assemblies];



   //infices to keep track of spawned TAOs
  int i = 0;
  int j = 0;
  int k = 0;

  int** input;
  int** output;
  for (int x = 0; x < dag_depth; x++)
  {
    // alternate input and output between steps of the DAG to make heat and matrix mult data-dependent
    if (x % 2)
    {
      input = matrix_input_a;
      output = matrix_output_c;
    } else {
      input = matrix_input_a;
      output = matrix_output_c;
    }
     
     //spawn Matrix multiplication taos
    for (int y = 0; y < matrix_width; y++)
    {
      matrix_ao[i] = new TAO_matrix(ma_width, //taowidth
                                    0, //start y
                                    stepsize, //stop y
                                    y*r_size, //start x
                                    stepsize+y*r_size, //stop x
                                    0, //output offset (if needed)
                                    ROW_SIZE, 
                                    input,
                                    matrix_input_b, 
                                    output);
      if (x == 0) {
        gotao_push_init(matrix_ao[i], i % nthreads);
      } else {
        matrix_ao[i-matrix_width]->make_edge(matrix_ao[i]);
      }
      i++;
    }

    for (int z = 0; z < sort_width; z++)
    {
      sort_ao[j] = new TAOQuickMergeDyn(matrix_output_c[z*2+r_size],
                                        matrix_output_c[z*2+1+r_size], 
                                        sort_size*BLOCKSIZE,
                                        sa_width);
      if (x == 0) {
        gotao_push_init(sort_ao[j], j % nthreads);
      } else {
        sort_ao[j-sort_width]->make_edge(sort_ao[j]);
      }
      j++;
    }

    for (int z = 0; z < heat_width; z++)
    {
      heat_ao[k] = new jacobi2D(
                             input[z+1+r_size+sort_width],
                             output[z+1+r_size+sort_width],
                             heat_resolution, heat_resolution,
                             0, // x*((np + exdecomp -1) / exdecomp),
                             0, //y*((np + eydecomp -1) / eydecomp),
                             heat_resolution, //(np + exdecomp - 1) / exdecomp,
                             heat_resolution, //(np + eydecomp - 1) / eydecomp, 
                             gotao_sched_2D_static,
                             heat_resolution/xdecomp, // (np + ixdecomp*exdecomp -1) / (ixdecomp*exdecomp),
                             heat_resolution/ydecomp, //(np + iydecomp*eydecomp -1) / (iydecomp*eydecomp), 
                             ha_width);
      if (x == 0) {
        gotao_push_init(heat_ao[k], k % nthreads);
      } else {
        heat_ao[k-heat_width]->make_edge(heat_ao[k]);
      }
      k++;
    }

  }
      




      /*
   	for(int x=0; x<ROW_SIZE; x+=stepsize){
  		for(int y=0; y<COL_SIZE; y+=stepsize){
              //level1[i] = new TAOMergeDyn(array1 + total_assemblies*i*2048, tmp + total_assemblies*i*2048, total_assemblies*2048, 2);

           		ao[i] = new TAO_matrix(2, x, x+stepsize, y, y+stepsize, ROW_SIZE, a, b, c); 
  	  	 	gotao_push_init(ao[i], i % nthreads);
          i++;
  		}	
    }
   
   */
   


   // measure execution time
std::cout << "starting \n";
   goTAO_start();

   goTAO_fini();

   end = std::chrono::system_clock::now();

   std::chrono::duration<double> elapsed_seconds = end-start;
   std::time_t end_time = std::chrono::system_clock::to_time_t(end);
 
   std::cout << "elapsed time: " << elapsed_seconds.count() << "s. " << "Total number of steals: " <<  tao_total_steals << "\n";
   std::cout << "Assembly Throughput: " << matrix_assemblies / elapsed_seconds.count() << " A/sec\n";
   std::cout << "Assembly Cycle: " << elapsed_seconds.count() / matrix_assemblies  << " sec/A\n";

  /*
  std::cout << matrix_output_c[0][0]<< " " << matrix_output_c[0][1] << " " << matrix_output_c[0][2] << " " << matrix_output_c[0][3] << "\n";
  
  std::cout << c[1][0] << c[1][1] << c[1][2] << c[1][3] << "\n";
  std::cout << c[2][0] << c[2][1] << c[2][2] << c[2][3] << "\n";
  std::cout << c[3][0] << c[3][1] << c[3][2] << c[3][3] << "\n";
  */
  for (int i = 0; i < ysize; ++i)
{
  delete matrix_input_a[i];
  delete matrix_input_b[i];
  delete matrix_output_c[i];
}
delete[] matrix_input_a;
delete[] matrix_input_b;
delete[] matrix_output_c;
   return (0);
}

void fill_arrays(int **a, int **b, int **c, int ysize, int xsize)
{

     //Fills the input matrices a and b with random integers and output matrix with 0
     for (int i = 0; i < ysize; ++i) { //each row
        for (int j = 0; j  < ysize; j++){ //each column
          a[i][j] = (rand() % 111);
          b[i][j] = (rand() % 111);
          c[i][j] = (rand() % 111);
        }
     }
}


//
//
//


#include <chrono>
#include <fstream>
#include <iostream>
#include <atomic>
#include <math.h>
#include "../../tao.h"
#include "../taomatrix.h"
#include "../solver-tao.h"
#include "../taosort.h"
#include "config-fork-join.h"



extern "C" {

#include <stdio.h>
#include <stdlib.h> 
#include <unistd.h>

}


#define BLOCKSIZE (2*1024)

//used to fill input data with random values
void fill_arrays(int **a, int **c, int ysize, int xsize);


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
  //internal x and y decompisition of copy
  int xdecomp; 
  int ydecomp;

  int ma_width; //matrix assembly width
  int sa_width; //sort assembly width
  int ha_width; //copy/heat assembly width

   int thread_base; int nthreads; int nctx; //Do we need the last parameter?
   int r_size; //row size of matrix multiplication
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

   if(getenv("DAG_DEPTH"))
  dag_depth=atoi(getenv("DAG_DEPTH"));
   else
  dag_depth=DAG_DEPTH;

   if(getenv("DAG_WIDTH"))
  dag_width=atoi(getenv("DAG_WIDTH"));
   else
  dag_width=DAG_WIDTH;

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


   if(matrix_width+sort_width+heat_width != 1) {
    std::cout << "One and only one of MATRIX_WIDTH, SORT_WIDTH and HEAT_WIDTH must be selected by setting it to 1 \n";
    return 0;
   }


/////MEMORY allocation part /////
   //1. Check which TAO class is selected, it should only be one
   //2. Allocate data in a 2d array depending on DAG_WIDTH and size of the selected TAO class

//if !matrix multiplication
  if (matrix_width == 0){
    r_size = 0;
  }
  int m_ysize = r_size;

  int m_xsize = (r_size * dag_width * 2);

int** matrix_input_a = new int* [m_ysize];
int** matrix_output_c = new int* [m_ysize];

for (int i = 0; i < m_ysize; ++i)
{
  matrix_input_a[i] = new int[m_xsize];
  matrix_output_c[i] = new int[m_xsize];
}



  int s_ysize = dag_width;
  int s_xsize = sort_size*BLOCKSIZE;


//if !sort
  if (sort_width == 0){
    s_ysize = 0;
  }

int** sort_input_a = new int* [s_ysize];
int** sort_output_c = new int* [s_ysize];

for (int i = 0; i < s_ysize; ++i)
{
  sort_input_a[i] = new int[s_xsize];
  sort_output_c[i] = new int[s_xsize];
}


  int h_ysize = dag_width+2;
  int h_xsize = heat_resolution*heat_resolution;



//if !copy/heat
  if (heat_width == 0){
    h_ysize = 0;
  }


int** heat_input_a = new int* [h_ysize];
int** heat_output_c = new int* [h_ysize];

for (int i = 0; i < h_ysize; ++i)
{
  heat_input_a[i] = new int[h_xsize];
  heat_output_c[i] = new int[h_xsize];
}

////MEMORY ALLOCATION DONE


    std::cout << "gotao_init parameters are: " << nthreads <<"," << thread_base << ","<< nctx << std::endl;

   gotao_init();








   // fill the input arrays and empty the output array
  fill_arrays(matrix_input_a , matrix_output_c, m_ysize, m_xsize);
  fill_arrays(sort_input_a , sort_output_c, s_ysize, s_xsize);
  fill_arrays(heat_input_a , heat_output_c, h_ysize, h_xsize);



   //Calcutation of how many TAOs we need to spawn
   int assembly_count = ( (dag_width + (dag_width-1) * 2 - 1) * (dag_depth / (log2(dag_width) * 2 )+1) );

   int matrix_assemblies = assembly_count * matrix_width;
   TAO_matrix *matrix_ao[matrix_assemblies];
   int sort_assemblies = assembly_count * sort_width;
   TAOQuickMergeDyn *sort_ao[sort_assemblies];
   int heat_assemblies = assembly_count * heat_width;
   copy2D *heat_ao[heat_assemblies];



   //indices to keep track of spawned TAOs
  int i = 0;
  int j = 0;
  int k = 0;

  //variables used to control DAG generation: current width and growing.
  int curr_width = 1;
  int growing = 1;

  //DOT graph printout
  std::ofstream graphfile;
  graphfile.open ("graph.txt");
  graphfile << "digraph DAG{\n";

  //loop through all levels of the DAG
  for (int x = 0; x < dag_depth; x++)
  {

     // check how many TAOs to spawn this level
     for (int y = 0; y <curr_width; y++)
     {
      //find in- and output data memory location
      int mem = (dag_width/curr_width)*y;


      if (matrix_width == 1)
      {
        matrix_ao[i] = new TAO_matrix(ma_width, //taowidth
                                      0, //start y
                                      r_size, //stop y
                                      mem*r_size*2, //start x
                                      r_size+mem*r_size*2, //stop x
                                      0, //output offset (if needed)
                                      ROW_SIZE, 
                                      matrix_input_a,
                                      matrix_output_c);
        //if this is the first TAO -> no input edges
        if (i == 0) { 
          gotao_push_init(matrix_ao[i], i % nthreads);
        } 
        //if the graph is growing -> one input edge
        else if (growing){  
          matrix_ao[i-((curr_width+y)/2)]->make_edge(matrix_ao[i]);
          graphfile << "  " << i-((curr_width+y+1)/2) << " -> " << i << " ;\n"; //DOT graph output
        }
        // if the graph is shrinking -> two input edges 
        else if (!growing){ 
          matrix_ao[i-(curr_width*2-y+1)]->make_edge(matrix_ao[i]);
          graphfile << "  " << i-(curr_width*2-y-1) << " -> " << i << " ;\n"; //DOT graph output 
          matrix_ao[i-(curr_width*2-y)]->make_edge(matrix_ao[i]);
          graphfile << "  " << i-(curr_width*2-y) << " -> " << i << " ;\n"; //DOT graph output
        }
        i++;
      }


      if (sort_width == 1)
      {
        sort_ao[j] = new TAOQuickMergeDyn(sort_input_a[mem],
                                          sort_output_c[mem], 
                                          sort_size*BLOCKSIZE,
                                          sa_width);
        //if this is the first TAO -> no input edges
        if (j == 0) { 
          gotao_push_init(sort_ao[j], j % nthreads);
        }
        //if the graph is growing -> one input edge
        else if (growing){
          sort_ao[j-((curr_width+y)/2)]->make_edge(sort_ao[j]);
          graphfile << "  " << j-((curr_width+y+1)/2) << " -> " << j << " ;\n";
        }
        // if the graph is shrinking -> two input edges 
        else {
          sort_ao[j-(curr_width*2-y+1)]->make_edge(sort_ao[j]);
          graphfile << "  " << j-(curr_width*2-y-1) << " -> " << j << " ;\n";
          sort_ao[j-(curr_width*2-y)]->make_edge(sort_ao[j]);
          graphfile << "  " << j-(curr_width*2-y) << " -> " << j << " ;\n";        
        }
        j++;
      }

      if (heat_width == 1)
      {
        heat_ao[k] = new copy2D(
                               heat_input_a[mem+1],
                               heat_output_c[mem+1],
                               heat_resolution, heat_resolution,
                               0, // x*((np + exdecomp -1) / exdecomp),
                               0, //y*((np + eydecomp -1) / eydecomp),
                               heat_resolution, //(np + exdecomp - 1) / exdecomp,
                               heat_resolution, //(np + eydecomp - 1) / eydecomp, 
                               gotao_sched_2D_static,
                               heat_resolution/xdecomp, // (np + ixdecomp*exdecomp -1) / (ixdecomp*exdecomp),
                               heat_resolution/ydecomp, //(np + iydecomp*eydecomp -1) / (iydecomp*eydecomp), 
                               ha_width);
        //if this is the first TAO -> no input edges
        if (k == 0) { 
          gotao_push_init(heat_ao[k], k % nthreads);
        } 
        //if the graph is growing -> one input edge
        else if (growing){
          heat_ao[k-((curr_width+y)/2)]->make_edge(heat_ao[k]);
          graphfile << "  " << k-((curr_width+y+1)/2) << " -> " << k << " ;\n";
        } 
        // if the graph is shrinking -> two input edges
        else {
          heat_ao[k-(curr_width*2-y+1)]->make_edge(heat_ao[k]);
          graphfile << "  " << k-(curr_width*2-y-1) << " -> " << k << " ;\n";
          heat_ao[k-(curr_width*2-y)]->make_edge(heat_ao[k]);
          graphfile << "  " << k-(curr_width*2-y) << " -> " << k << " ;\n";        
        }
        k++;
      }


    }

    // if the graph is growing and current width is smaller than maximum width -> keep growing
    if (growing && (curr_width < dag_width)){
      curr_width = curr_width*2;
      //if the current width now exceeds the maximum width -> set it to maxumum width
      if (curr_width > dag_width){
        curr_width = dag_width;
      }
    }

    //if the graph is growing and current width is equal to maximum width -> start shrinking
    else if (growing && (curr_width == dag_width)){
      growing = 0;
      curr_width = curr_width/2;
    }

    //if the graph is shrinking and current width is more than one (minimum width) -> keep shrinking
    else if (!growing && (curr_width > 1)){
      curr_width = curr_width/2;
      //if the current width now is less than one (somehow?) -> set to one
      if (curr_width < 1) {
        curr_width == 1;
      }
    }

    //if the graph is shrinking and current width is 1 -> start growing.
    else if (!growing && (curr_width ==1)){
      growing = 1;
      curr_width = curr_width *2;
    }
    

  }

  //close output file for DOT graph
  graphfile << "}";
  graphfile.close();




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



//Free memory

  for (int i = 0; i < m_ysize; ++i)
{
  delete matrix_input_a[i];
  delete matrix_output_c[i];
}
delete[] matrix_input_a;
delete[] matrix_output_c;


  for (int i = 0; i < s_ysize; ++i)
{
  delete sort_input_a[i];
  delete sort_output_c[i];
}
delete[] sort_input_a;
delete[] sort_output_c;


  for (int i = 0; i < h_ysize; ++i)
{
  delete heat_input_a[i];
  delete heat_output_c[i];
}
delete[] heat_input_a;
delete[] heat_output_c;


   return (0);
}



void fill_arrays(int **a, int **c, int ysize, int xsize)
{

     //Fills the matrices a and c with random integer
     for (int i = 0; i < ysize; ++i) { //each row
        for (int j = 0; j  < ysize; j++){ //each column
          a[i][j] = (rand() % 111);
          c[i][j] = (rand() % 111);
        }
     }
}


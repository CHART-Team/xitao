//
//
//


#include <chrono>
#include <fstream>
#include <iostream>
#include <atomic>
#include <vector>
#include <algorithm>
#include "../../tao.h"
#include "../taomatrix.h"
#include "../solver-tao.h"
#include "../taosort.h"
#include "randombench.h"
#include "config-random-bench.h"



extern "C" {

#include <stdio.h>
#include <stdlib.h> 
#include <unistd.h>

}


#define BLOCKSIZE (2*1024)


void fill_arrays(int **a, int **c, int ysize, int xsize);


   std::vector<node> nodes;

// MAIN 
int
main(int argc, char* argv[])
{

  int matrix_count; //number of matrix multiplication TAOs
  int sort_count; //sort TAOs count
  int heat_count; //heat/copy TAO count
  int dag_width; 
  int sort_size; 
  int heat_resolution;
  int xdecomp;
  int ydecomp;
  int ma_width; //matrix assembly width
  int sa_width; //sort assembly width
  int ha_width; //heat/copy assembly width


  srand(R_SEED);
  int currentheatcount;
  int currentsortcount;
  int currentmatrixcount;

  int edge_rate;
  int level_width; //average number of TAOs spawned in parallel


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

   if(getenv("EDGE_RATE"))
  edge_rate=atoi(getenv("EDGE_RATE"));
   else
  edge_rate=EDGE_RATE;

   if(getenv("*LEVEL_WIDTH"))
  level_width=atoi(getenv("LEVEL_WIDTH"));
   else
  level_width=LEVEL_WIDTH;

   if(getenv("MATRIX_COUNT"))
  matrix_count=atoi(getenv("MATRIX_COUNT"));
   else
  matrix_count=MATRIX_COUNT;

  if(getenv("SORT_COUNT"))
  sort_count=atoi(getenv("SORT_COUNT"));
   else
  sort_count=SORT_COUNT;

  if(getenv("SORT_SIZE"))
  sort_size=atoi(getenv("SORT_SIZE"));
   else
  sort_size=SORT_SIZE;

  if(getenv("HEAT_COUNT"))
  heat_count=atoi(getenv("HEAT_COUNT"));
   else
  heat_count=HEAT_COUNT;

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

  if(getenv("R_SEED"))
    srand(R_SEED);

   if((COL_SIZE != ROW_SIZE) || (0!=(COL_SIZE % stepsize))){
	std::cout << "Incompatible matrix parameters, please choose ROW_SIZE=COL_SIZE and a stepsize that is a divisior of the ROW_SIZE&&COL_SIZE";
	return 0;
   }

   if(level_width<1){//cant have level width less than one
  std::cout << "level_width must be > 0";
  return 0;
   }

   currentmatrixcount = matrix_count;
   currentsortcount   = sort_count;
   currentheatcount   = heat_count;

   int nodecount = 0;
   int ic = 0;


   std::cout << "prewhile \n";


   std::vector<int> matrix_mem;
   std::vector<int> sort_mem;
   std::vector<int> heat_mem;

   std::ofstream graphfile;
   graphfile.open ("graph.txt");
   graphfile << "digraph DAG{\n";

  while(nodecount < (heat_count + sort_count + matrix_count)){
    int x;
    int w = (rand() % (level_width*2-1)+1);
    ic = 0;

    while (ic<w && (currentmatrixcount+currentsortcount+currentheatcount)){

      Taotype nodetype;
      int taonumber;
      x = rand() % (currentheatcount + currentsortcount + currentmatrixcount);



      if ( ((0 <= x) && (x < currentheatcount)) ){
        nodetype = heat;
        taonumber = heat_count - currentheatcount;
        currentheatcount--;
      }else if ((currentheatcount <= x) && (x < (currentheatcount + currentsortcount))){
        nodetype = sort_;
        taonumber = sort_count - currentsortcount;
        currentsortcount--;
      }else { 
        nodetype = matrix;
        taonumber = matrix_count - currentmatrixcount;
        currentmatrixcount--;
      }

      std::cout << "creating node #" << nodecount+ic <<", w is: " << w << "\n" ;
      nodes.push_back (new_node(nodetype, //matrix, sort or heat
                                nodecount+ic,
                                taonumber));



      for (int y = 1; y <= (min(30, nodecount)); y++){
        if ((rand() % 100) < edge_rate){
          if (!edge_check(nodes[nodecount-y].nodenr, nodes[nodecount+ic])){
            add_edge(nodes[nodecount+ic], //the newly created node
                     nodes[nodecount-y]); // the node to add
            graphfile << "  " << nodecount-y << " -> " << nodecount+ic << " ;\n";
          }
        } 
      }

      int a;
      switch(nodetype){
        case matrix :
          a = find_mem(nodetype, nodecount+ic, nodes[nodecount+ic], matrix_mem);
          add_space(nodes[nodecount+ic], matrix_mem, a);
          break;
        case sort_ :
          a = find_mem(nodetype, nodecount+ic, nodes[nodecount+ic], sort_mem);
          add_space(nodes[nodecount+ic], sort_mem, a);
          break;
        case heat :
          a = find_mem(nodetype, nodecount+ic, nodes[nodecount+ic], heat_mem);
          add_space(nodes[nodecount+ic], heat_mem, a);
      } 


      ic++;

    }

    nodecount = nodecount + ic;
    //std::cout <<"end of while iter\n";
    //std::cout <<"ic: " << ic;
    //std::cout <<"\nw: " << w << "\n";
    std::cout << "nodecount: " << nodecount << "\n";
  }
  for (int i = 0; i < nodes.size(); i++){
    switch(nodes[i].ttype) {
      case matrix :
        graphfile << nodes[i].nodenr << "  [fillcolor = lightpink, style = filled];\n";
        break;
      case sort_ : 
        graphfile << nodes[i].nodenr << "  [fillcolor = skyblue, style = filled];\n";
        break;
      case heat :
        graphfile << nodes[i].nodenr << "  [fillcolor = palegreen, style = filled];\n";
        break;
    }
  }
  graphfile << "}";
  graphfile.close();


std::cout << "martix_mem size: " << matrix_mem.size() << "\n";
std::cout << "sort_mem size:   " << sort_mem.size() << "\n";
std::cout << "heat_mem size:   " << heat_mem.size() << "\n";

  if (matrix_mem.size() == 0){
    r_size = 0;
  }
  int m_ysize = r_size;

  int m_xsize = (r_size * matrix_mem.size() * 2);

int** matrix_input_a = new int* [m_ysize];
int** matrix_output_c = new int* [m_ysize];

for (int i = 0; i < m_ysize; ++i)
{
  matrix_input_a[i] = new int[m_xsize];
  matrix_output_c[i] = new int[m_xsize];
}



  int s_ysize = (sort_mem.size()) + 4;
  int s_xsize = sort_size*BLOCKSIZE;

int** sort_input_a = new int* [s_ysize];
int** sort_output_c = new int* [s_ysize];

for (int i = 0; i < s_ysize; ++i)
{
  sort_input_a[i] = new int[s_xsize];
  sort_output_c[i] = new int[s_xsize];
}



  int h_ysize = (heat_mem.size())+4;
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


    std::cout <<"arrays filled\n";
   //Spawn TAOs for every seperately written too value, equivilent to for(i->i+stepsize){for(j->j+stepsize) generation}
   //int matrix_assemblies = dag_depth * matrix_width;
   TAO_matrix *matrix_ao[matrix_count];
   //int sort_assemblies = dag_depth * sort_width;
   TAOQuickMergeDyn *sort_ao[sort_count];
   //int heat_assemblies = dag_depth * heat_width;
   copy2D *heat_ao[heat_count];






   //infices to keep track of spawned TAOs
  int i = 0;
  int j = 0;
  int k = 0;



  for (int x = 0; x < nodecount; x++)
  {
    // alternate input and output between steps of the DAG to make heat and matrix mult data-dependent
     
     //spawn Matrix multiplication taos
    if (nodes[x].ttype == matrix)
    {
      matrix_ao[i] = new TAO_matrix(ma_width, //taowidth
                                    0, //start y
                                    stepsize, //stop y
                                    (nodes[x].mem_space)*r_size*2, //start x
                                    stepsize+(nodes[x].mem_space)*r_size*2, //stop x
                                    0, //output offset (if needed)
                                    ROW_SIZE, 
                                    matrix_input_a,
                                    matrix_output_c);
      if (nodes[x].edges.size() == 0) {
        gotao_push(matrix_ao[i]);
      } else {
        for (int y = 0; y < (nodes[x].edges).size(); y++) {
          switch (nodes[((nodes[x]).edges[y])].ttype) {
            case matrix :
              matrix_ao[nodes[((nodes[x]).edges[y])].taonr]->make_edge(matrix_ao[i]);
              break;
            case sort_ :
              sort_ao[nodes[((nodes[x]).edges[y])].taonr]->make_edge(matrix_ao[i]);
              break;
            case heat :
              heat_ao[nodes[((nodes[x]).edges[y])].taonr]->make_edge(matrix_ao[i]);
              break;
          }
        }
      }
      i++;
    }

    if (nodes[x].ttype == sort_)
    {
      sort_ao[j] = new TAOQuickMergeDyn(sort_input_a[nodes[x].mem_space],
                                        sort_output_c[nodes[x].mem_space], 
                                        sort_size*BLOCKSIZE,
                                        sa_width);
      if (nodes[x].edges.size() == 0) {
        gotao_push(sort_ao[j]);
      } else {
        for (int y = 0; y < (nodes[x].edges).size(); y++) {
          switch (nodes[((nodes[x]).edges[y])].ttype) {
            case matrix :
              matrix_ao[nodes[((nodes[x]).edges[y])].taonr]->make_edge(sort_ao[j]);
              break;
            case sort_ :
              sort_ao[nodes[((nodes[x]).edges[y])].taonr]->make_edge(sort_ao[j]);
              break;
            case heat :
              heat_ao[nodes[((nodes[x]).edges[y])].taonr]->make_edge(sort_ao[j]);
              break;
          }
        }
      }
      j++;
    }

    if (nodes[x].ttype == heat)
    {
      heat_ao[k] = new copy2D(
                             heat_input_a[(nodes[x].mem_space)+1],
                             heat_output_c[(nodes[x].mem_space)+1],
                             heat_resolution, heat_resolution,
                             0, // x*((np + exdecomp -1) / exdecomp),
                             0, //y*((np + eydecomp -1) / eydecomp),
                             heat_resolution, //(np + exdecomp - 1) / exdecomp,
                             heat_resolution, //(np + eydecomp - 1) / eydecomp, 
                             gotao_sched_2D_static,
                             heat_resolution/xdecomp, // (np + ixdecomp*exdecomp -1) / (ixdecomp*exdecomp),
                             heat_resolution/ydecomp, //(np + iydecomp*eydecomp -1) / (iydecomp*eydecomp), 
                             ha_width);
      if (nodes[x].edges.size() == 0) {
        gotao_push(heat_ao[k]);
      } else {
        for (int y = 0; y < (nodes[x].edges).size(); y++) {
          switch (nodes[((nodes[x]).edges[y])].ttype) {
            case matrix :
              matrix_ao[nodes[((nodes[x]).edges[y])].taonr]->make_edge(heat_ao[k]);
              break;
            case sort_ :
              sort_ao[nodes[((nodes[x]).edges[y])].taonr]->make_edge(heat_ao[k]);
              break;
            case heat :
              heat_ao[nodes[((nodes[x]).edges[y])].taonr]->make_edge(heat_ao[k]);
              break;
          }
        }
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
std::cout << "i = " <<  i << "\n";
std::cout << "matrix_count = " << matrix_count << "\n";
std::cout << "j = " <<  j << "\n";
std::cout << "sort_count = " << sort_count << "\n";
std::cout << "k = " <<  k << "\n";
std::cout << "heat_count = " << heat_count << "\n";

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
   std::cout << "Assembly Throughput: " << (nodecount) / elapsed_seconds.count() << " A/sec\n";
   std::cout << "Assembly Cycle: " << elapsed_seconds.count() / (nodecount)  << " sec/A\n";

  /*
  std::cout << matrix_output_c[0][0]<< " " << matrix_output_c[0][1] << " " << matrix_output_c[0][2] << " " << matrix_output_c[0][3] << "\n";
  
  std::cout << c[1][0] << c[1][1] << c[1][2] << c[1][3] << "\n";
  std::cout << c[2][0] << c[2][1] << c[2][2] << c[2][3] << "\n";
  std::cout << c[3][0] << c[3][1] << c[3][2] << c[3][3] << "\n";
  */
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
         // b[i][j] = (rand() % 111);
          c[i][j] = (rand() % 111);
        }
     }
}



node new_node(Taotype type, int nodenr, int taonr){
  node n;
  n.ttype = type;
  n.nodenr = nodenr;
  n.taonr = taonr;
  //std::vector<int> v = {};
  //n.edges = v;
  return n;
}

void add_edge(node &n, node const &e){
  (n.edges).push_back (e.nodenr);
}

void add_space(node &n, std::vector<int> &v, int a){
  if (a == -1){
    v.push_back (n.nodenr);
    n.mem_space = (v.size() + 1);
  } else {
  n.mem_space = a;
  }
}

int find_mem(Taotype type, int nodenr, node const &n, std::vector<int> &v){
  int minnr = nodenr;
  //if (n.ttype == type) {
    
    for (int i = 0; i < v.size(); i++) {
      minnr = min(minnr, v[i]);
      if (n.nodenr == v[i] && n.ttype == type) {
        v[i] = nodenr;
        return i;
      }
    }
  //}
  //std::cout << "nodenr: " << n.nodenr << "    minnr: " << minnr << "\n";
  int x;
  for (int i = 0; i < (n.edges).size(); i++) {
    if (nodes[n.edges[i]].nodenr >= minnr) {
      x = find_mem(type, nodenr, nodes[n.edges[i]], v);
    if (x > -1) {
      return x;
    }
    
    }
  }

  return -1;

}


bool edge_check(int edge, node const &n){
  //std::cout << edge << " " << n.nodenr << "\n";
  if (edge <= n.nodenr){
    if (edge == n.nodenr){
      return true;
    }
    else{
      for (int i = 0; i < (n.edges).size(); i++) {
        if (edge_check(edge, nodes[n.edges[i]])) {
          return true;
        }
      }
    }
  }
  return false;
}


